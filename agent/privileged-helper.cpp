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
    if (arguments.size() != 4 || arguments.at(1) != QStringLiteral("--install-deb"))
        return refuse(QStringLiteral("only --install-deb <artifact> <service> is accepted"));
    const QString artifact = QFileInfo(arguments.at(2)).canonicalFilePath();
    const QString service = arguments.at(3);
    static const QRegularExpression unit(QStringLiteral("^[a-z][a-z0-9-]{1,63}$"));
    if (artifact.isEmpty() || !artifact.startsWith(QString::fromLatin1(kDownloads))
        || !artifact.endsWith(QStringLiteral(".deb")) || !unit.match(service).hasMatch()
        || !declaredService(service)) {
        return refuse(QStringLiteral("artifact or declared service is invalid"));
    }
    // dpkg et systemctl testent getuid() (UID réel), pas l'euid. Tant que le
    // helper n'a que l'euid root, dpkg sort tout de suite (« superuser privilege »)
    // alors que le même .deb s'installe avec sudo. Même classe d'erreur que
    // mount.cifs. On devient root réel après les contrôles ci-dessus.
    if (setgid(0) != 0 || setuid(0) != 0)
        return refuse(QStringLiteral("cannot assume real root"));
    QString detail;
    if (!run(QStringLiteral("/usr/bin/dpkg"),
             {QStringLiteral("--install"), artifact}, &detail))
        return refuse(QStringLiteral("dpkg failed: ") + detail);
    if (!run(QStringLiteral("/usr/bin/systemctl"),
             {QStringLiteral("restart"), service}, &detail)
        || !run(QStringLiteral("/usr/bin/systemctl"),
                {QStringLiteral("is-active"), QStringLiteral("--quiet"), service},
                &detail)) {
        return refuse(QStringLiteral("service did not restart: ") + detail);
    }
    return 0;
#endif
}
