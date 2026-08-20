#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#include <aclapi.h>
#include <accctrl.h>
#include <sddl.h>
#endif

#include "morfupdate/AgentConfig.h"
#include "morfupdate/LocalApiServer.h"
#include "morfupdate/OperationStore.h"
#include "morfupdate/UpdateEngine.h"

namespace {

QString defaultConfigPath() {
#ifdef Q_OS_WIN
    return QDir(qEnvironmentVariable("ProgramData")).filePath(
        QStringLiteral("morfsystem/morfupdate/morfupdate.json"));
#else
    return QStringLiteral("/etc/morfsystem/morfupdate/morfupdate.json");
#endif
}

QString stateDirectory() {
#ifdef Q_OS_WIN
    return QDir(qEnvironmentVariable("ProgramData")).filePath(
        QStringLiteral("morfsystem/morfupdate/state"));
#else
    const QString fromSystemd = qEnvironmentVariable("STATE_DIRECTORY").split(':').value(0);
    if (!fromSystemd.isEmpty())
        return fromSystemd;
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("morfupdate"));
#endif
}

#ifdef Q_OS_WIN
bool restrictWindowsToken(const QString& path, QString* error) {
    PSID systemSid = nullptr;
    if (!ConvertStringSidToSidW(L"S-1-5-18", &systemSid)) {
        *error = QStringLiteral("cannot resolve the SYSTEM account SID");
        return false;
    }
    EXPLICIT_ACCESSW entry{};
    entry.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
    entry.grfAccessMode = SET_ACCESS;
    entry.grfInheritance = NO_INHERITANCE;
    entry.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entry.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    entry.Trustee.ptstrName = static_cast<LPWSTR>(systemSid);
    PACL acl = nullptr;
    const DWORD aclResult = SetEntriesInAclW(1, &entry, nullptr, &acl);
    LocalFree(systemSid);
    if (aclResult != ERROR_SUCCESS) {
        *error = QStringLiteral("cannot create the SYSTEM-only token ACL");
        return false;
    }
    const DWORD result = SetNamedSecurityInfoW(
        const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(path.utf16())), SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
        nullptr, nullptr, acl, nullptr);
    LocalFree(acl);
    if (result != ERROR_SUCCESS) {
        *error = QStringLiteral("cannot protect the token ACL");
        return false;
    }
    return true;
}
#endif

QString configDirectory() {
#ifdef Q_OS_WIN
    return QDir(qEnvironmentVariable("ProgramData")).filePath(
        QStringLiteral("morfsystem/morfupdate"));
#else
    return QStringLiteral("/etc/morfsystem/morfupdate");
#endif
}

void resolveProtectedPaths(morfupdate::AgentConfig* config) {
    config->tokenFile.replace(QStringLiteral("@morfupdate-state@"), stateDirectory());
    config->githubTokenFile.replace(QStringLiteral("@morfupdate-config@"), configDirectory());
}

bool ensureToken(const QString& path, QString* error) {
    QFile existing(path);
    if (existing.exists()) {
#ifndef Q_OS_WIN
        const QFile::Permissions unsafe = QFileDevice::ReadGroup | QFileDevice::WriteGroup
            | QFileDevice::ReadOther | QFileDevice::WriteOther;
        if (existing.permissions() & unsafe) {
            *error = QStringLiteral("token file permissions expose it beyond its service identities");
            return false;
        }
#endif
#ifdef Q_OS_WIN
        if (!restrictWindowsToken(path, error))
            return false;
#endif
        return true;
    }
    if (!QDir().mkpath(QFileInfo(path).dir().path())) {
        *error = QStringLiteral("cannot create token directory");
        return false;
    }
    QByteArray random;
    for (int index = 0; index < 4; ++index) {
        const quint64 value = QRandomGenerator::system()->generate64();
        random.append(QByteArray::number(value, 16).rightJustified(16, '0'));
    }
    QSaveFile token(path);
    if (!token.open(QIODevice::WriteOnly)) {
        *error = token.errorString();
        return false;
    }
    token.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    token.write(random + '\n');
    if (!token.commit()) {
        *error = token.errorString();
        return false;
    }
#ifdef Q_OS_WIN
    if (!restrictWindowsToken(path, error))
        return false;
#endif
    return true;
}

void errorLine(const QString& value) {
    QTextStream stream(stderr);
    stream << value << '\n';
    stream.flush();
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("morfUpdate"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption configOption({"c", "config"}, QStringLiteral("Agent configuration file."),
                                    QStringLiteral("path"));
    parser.addOption(configOption);
    parser.process(app);

    morfupdate::AgentConfig config;
    QString error;
    const QString configPath = parser.value(configOption).isEmpty()
        ? defaultConfigPath() : parser.value(configOption);
    if (!morfupdate::AgentConfig::load(configPath, &config, &error)) {
        errorLine(QStringLiteral("morfUpdate configuration refused: ") + error);
        return 2;
    }
    resolveProtectedPaths(&config);
    if (!ensureToken(config.tokenFile, &error)) {
        errorLine(QStringLiteral("morfUpdate token refused: ") + error);
        return 3;
    }
    morfupdate::OperationStore operations(stateDirectory());
    if (!operations.load(&error)) {
        errorLine(QStringLiteral("morfUpdate operation journal refused: ") + error);
        return 4;
    }
    morfupdate::LocalApiServer api(config, &operations);
    if (!api.start(&error)) {
        errorLine(QStringLiteral("morfUpdate API refused: ") + error);
        return 5;
    }
    morfupdate::UpdateEngine engine(config, &operations, stateDirectory());
    QObject::connect(&api, &morfupdate::LocalApiServer::operationQueued,
                     &engine, &morfupdate::UpdateEngine::run, Qt::QueuedConnection);
    QTextStream(stdout) << "morfUpdate agent listening on 127.0.0.1:" << api.port() << '\n';
    return app.exec();
}
