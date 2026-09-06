/*
 * morfUpdate - deliberately narrow Linux privilege boundary
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTextStream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace {

constexpr auto kConfig = "/etc/morfsystem/morfupdate/morfupdate.json";
constexpr auto kDownloads = "/var/lib/morfsystem/morfupdate/downloads/";

bool declaredService(const QString& service) {
    QFile file(QString::fromLatin1(kConfig));
    if (!file.open(QIODevice::ReadOnly)) return false;
    const QJsonDocument config = QJsonDocument::fromJson(file.readAll());
    for (const QJsonValue& value : config.object().value("targets").toArray()) {
        if (value.toObject().value("service").toString() == service) return true;
    }
    return false;
}

int refuse(const QString& message) {
    QTextStream(stderr) << "morfUpdate helper refused: " << message << '\n';
    return 2;
}

bool run(const QString& program, const QStringList& arguments, QString* detail) {
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // Sans ça, dpkg peut attendre une question sur un conffile et échouer.
    env.insert(QStringLiteral("DEBIAN_FRONTEND"), QStringLiteral("noninteractive"));
    process.setProcessEnvironment(env);
    process.start(program, arguments);
    if (!process.waitForStarted(10000) || !process.waitForFinished(-1)
        || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (detail) {
            *detail = err.isEmpty()
                          ? QStringLiteral("%1 failed (code %2)")
                                .arg(program)
                                .arg(process.exitCode())
                          : err;
        }
        return false;
    }
    return true;
}

#ifdef Q_OS_UNIX
// Recharge systemd et (re)démarre le service, avec quelques tentatives : un
// /healthz lent ou un enable qui a avalé une erreur ne doit pas figer le service
// éteint. Renvoie vrai quand l'unité est active. Factorisé : deb et bundle en ont
// besoin à l'identique.
bool bringUp(const QString& service, QString* detail) {
    QString ignored;
    run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("daemon-reload")}, &ignored);
    run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("reset-failed"), service}, &ignored);
    run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("enable"), service}, &ignored);
    bool active = false;
    for (int attempt = 0; attempt < 4 && !active; ++attempt) {
        if (attempt > 0) sleep(2);
        if (!run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("restart"), service}, detail))
            continue;
        active = run(QStringLiteral("/usr/bin/systemctl"),
                     {QStringLiteral("is-active"), QStringLiteral("--quiet"), service}, detail);
    }
    return active;
}

// Relance un service DEJA installe, sans rien reinstaller : efface un eventuel
// etat « failed », redemarre, puis confirme l'activite. C'est l'action du bouton
// « Relancer » de morfMonitor pour un service detecte bloque. On NE reutilise pas
// bringUp (qui fait daemon-reload + enable, utiles apres une install) : un restart
// est un restart. Le service a deja ete valide (regex + declaredService) et n'est
// jamais un chemin ni une commande arbitraire -- seulement un nom de service declare.
int restartService(const QString& service) {
    QString detail, ignored;
    run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("reset-failed"), service}, &ignored);
    if (!run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("restart"), service}, &detail))
        return refuse(QStringLiteral("service did not restart: ") + detail);
    if (!run(QStringLiteral("/usr/bin/systemctl"),
             {QStringLiteral("is-active"), QStringLiteral("--quiet"), service}, &detail))
        return refuse(QStringLiteral("service is not active after restart: ") + detail);
    return 0;
}

// Installe un source-bundle DEJA EXTRAIT : échange atomique du répertoire
// applicatif sous /opt, avec sauvegarde pour rollback. Le helper ne touche
// jamais à l'archive elle-même (extraite non privilégiée par l'agent) ; il ne
// fait que des déplacements de répertoires et un redémarrage, tous auditables.
// Le répertoire cible est une CONVENTION (/opt/<service>), jamais un chemin reçu
// de l'appelant : rien d'arbitraire ne peut être écrasé.
int installBundle(const QString& unpack, const QString& service) {
    const QString appDir = QStringLiteral("/opt/") + service;
    const QString backup = appDir + QStringLiteral(".morfupdate.bak");
    QString detail, ignored;

    // Le service peut tenir des fichiers ouverts : on l'arrête avant l'échange.
    run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("stop"), service}, &ignored);

    run(QStringLiteral("/usr/bin/rm"), {QStringLiteral("-rf"), backup}, &ignored);
    const bool hadPrevious = QFileInfo::exists(appDir);
    if (hadPrevious && !run(QStringLiteral("/usr/bin/mv"), {appDir, backup}, &detail)) {
        bringUp(service, &ignored);   // ne pas laisser le service à terre
        return refuse(QStringLiteral("cannot set aside current install: ") + detail);
    }
    // cp -aT : copie le CONTENU de unpack dans appDir (crée appDir), en
    // préservant les modes. On ne déplace pas unpack (il vit dans downloads/,
    // nettoyé par ailleurs) pour garder la sauvegarde intacte en cas d'échec.
    if (!run(QStringLiteral("/usr/bin/cp"),
             {QStringLiteral("-aT"), unpack, appDir}, &detail)) {
        run(QStringLiteral("/usr/bin/rm"), {QStringLiteral("-rf"), appDir}, &ignored);
        if (hadPrevious) run(QStringLiteral("/usr/bin/mv"), {backup, appDir}, &ignored);
        bringUp(service, &ignored);
        return refuse(QStringLiteral("cannot deploy source bundle: ") + detail);
    }
    // Conserver le propriétaire établi (l'utilisateur du service), repris de la
    // sauvegarde : le service lit son code sous cette identité.
    if (hadPrevious) {
        run(QStringLiteral("/usr/bin/chown"),
            {QStringLiteral("-R"), QStringLiteral("--reference=") + backup, appDir}, &ignored);
    }

    if (!bringUp(service, &detail)) {
        // Rollback : restaurer l'ancienne arborescence et relancer.
        run(QStringLiteral("/usr/bin/rm"), {QStringLiteral("-rf"), appDir}, &ignored);
        if (hadPrevious) run(QStringLiteral("/usr/bin/mv"), {backup, appDir}, &ignored);
        bringUp(service, &ignored);
        return refuse(QStringLiteral("service did not restart; rolled back: ") + detail);
    }
    run(QStringLiteral("/usr/bin/rm"), {QStringLiteral("-rf"), backup}, &ignored);
    return 0;
}
#endif  // Q_OS_UNIX

} // namespace

int main(int argc, char** argv) {
    // Ce binaire est setuid (4750) : lance par le service non privilegie, son euid
    // passe a root. Qt refuse par defaut de tourner setuid (« running setuid, this
    // is a security hole ») et avorte. On l'autorise explicitement, AVANT de
    // construire QCoreApplication : c'est le mecanisme voulu, borne par les
    // controles ci-dessous (root requis, verbe unique, service declare).
    QCoreApplication::setSetuidAllowed(true);
    QCoreApplication app(argc, argv);
#ifndef Q_OS_UNIX
    return refuse(QStringLiteral("the privileged helper is only used on Linux"));
#else
    if (geteuid() != 0) return refuse(QStringLiteral("root execution is required"));
    const QStringList arguments = app.arguments();
    static const QRegularExpression unit(QStringLiteral("^[a-z][a-z0-9-]{1,63}$"));

    // Verbe --restart : forme COURTE <verbe> <service>, sans source ni fichier.
    // Périmètre volontairement minuscule : relancer un service DÉJÀ déclaré, rien
    // d'autre. Mêmes barrières que l'install (regex d'unité + declaredService),
    // passage à root SEULEMENT après validation. Jamais de commande arbitraire.
    if (arguments.value(1) == QStringLiteral("--restart")) {
        if (arguments.size() != 3)
            return refuse(QStringLiteral("usage: --restart <service>"));
        const QString service = arguments.at(2);
        if (!unit.match(service).hasMatch() || !declaredService(service))
            return refuse(QStringLiteral("declared service is invalid"));
        if (setgid(0) != 0 || setuid(0) != 0)
            return refuse(QStringLiteral("cannot assume real root"));
        return restartService(service);
    }

    // Deux verbes, même forme : <verbe> <source> <service>.
    //   --install-deb    <artifact.deb>  <service>  (projet compilé)
    //   --install-bundle <unpack-dir>    <service>  (projet source-bundle)
    if (arguments.size() != 4
        || (arguments.at(1) != QStringLiteral("--install-deb")
            && arguments.at(1) != QStringLiteral("--install-bundle"))) {
        return refuse(QStringLiteral(
            "usage: --install-deb <artifact> <service> | --install-bundle <unpackdir> <service>"));
    }
    const QString verb = arguments.at(1);
    const QString service = arguments.at(3);
    if (!unit.match(service).hasMatch() || !declaredService(service))
        return refuse(QStringLiteral("declared service is invalid"));

    // Validation de la source AVANT de devenir root. Dans les deux cas la source
    // doit vivre sous le répertoire de téléchargements protégé de l'agent.
    QString artifact;
    QString unpack;
    if (verb == QStringLiteral("--install-deb")) {
        artifact = QFileInfo(arguments.at(2)).canonicalFilePath();
        if (artifact.isEmpty() || !artifact.startsWith(QString::fromLatin1(kDownloads))
            || !artifact.endsWith(QStringLiteral(".deb")))
            return refuse(QStringLiteral("artifact is invalid"));
    } else {
        unpack = QFileInfo(arguments.at(2)).canonicalFilePath();
        if (unpack.isEmpty() || !unpack.startsWith(QString::fromLatin1(kDownloads))
            || !QFileInfo::exists(unpack + QStringLiteral("/VERSION")))
            return refuse(QStringLiteral("unpack directory is invalid"));
    }

    // dpkg et systemctl testent getuid() (UID réel), pas l'euid. Tant que le
    // helper n'a que l'euid root, dpkg sort tout de suite (« superuser privilege »)
    // alors que le même .deb s'installe avec sudo. Même classe d'erreur que
    // mount.cifs. On devient root réel après les contrôles ci-dessus.
    if (setgid(0) != 0 || setuid(0) != 0)
        return refuse(QStringLiteral("cannot assume real root"));

    if (verb == QStringLiteral("--install-bundle"))
        return installBundle(unpack, service);

    // --- install-deb : installation du paquet puis (re)démarrage ---
    QString detail;
    if (!run(QStringLiteral("/usr/bin/dpkg"),
             {QStringLiteral("--install"), artifact}, &detail))
        return refuse(QStringLiteral("dpkg failed: ") + detail);
    // L'ancien prerm faisait disable --now a chaque upgrade. Si le postinst
    // avale un echec d'enable (--now || true), le service reste eteint : cas vu
    // en mettant a jour morfMonitor depuis sa propre page (Failed to fetch).
    if (!bringUp(service, &detail))
        return refuse(QStringLiteral("service did not restart: ") + detail);
    return 0;
#endif
}
