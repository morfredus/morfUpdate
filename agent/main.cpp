#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

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
