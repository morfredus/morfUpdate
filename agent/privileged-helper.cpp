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

bool run(const QString& program, const QStringList& arguments) {
    QProcess process;
    process.start(program, arguments);
    return process.waitForStarted(10000) && process.waitForFinished(-1)
        && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

int refuse(const QString& message) {
    QTextStream(stderr) << "morfUpdate helper refused: " << message << '\n';
    return 2;
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
    if (!run(QStringLiteral("dpkg"), {QStringLiteral("--install"), artifact}))
        return refuse(QStringLiteral("dpkg failed"));
    if (!run(QStringLiteral("systemctl"), {QStringLiteral("restart"), service})
        || !run(QStringLiteral("systemctl"), {QStringLiteral("is-active"), QStringLiteral("--quiet"), service})) {
        return refuse(QStringLiteral("service did not restart"));
    }
    return 0;
#endif
}
