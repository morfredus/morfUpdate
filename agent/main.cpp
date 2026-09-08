#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>

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
    QCoreApplication::setApplicationVersion(QStringLiteral(MORFUPDATE_VERSION));

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
    // L'installation (dpkg, attente du redemarrage, sondes de sante) est longue et
    // bloquante. Si le moteur tournait sur le thread principal, elle gelerait le
    // serveur HTTP : /healthz et le suivi GET /api/v1/updates/<id> deviendraient
    // muets pendant toute la mise a jour, exactement au moment ou un superviseur
    // veut lire la progression. On deporte donc le moteur sur un thread worker.
    // Les connexions restent Qt::QueuedConnection : le signal est emis sur le
    // thread HTTP, le slot s'execute sur le worker (affinite du moteur). Le journal
    // partage (OperationStore) est protege par mutex, seul point de contact.
    morfupdate::UpdateEngine engine(config, &operations, stateDirectory());
    QThread worker;
    engine.moveToThread(&worker);
    worker.start();
    QObject::connect(&api, &morfupdate::LocalApiServer::operationQueued,
                     &engine, &morfupdate::UpdateEngine::run, Qt::QueuedConnection);
    QObject::connect(&api, &morfupdate::LocalApiServer::restartQueued,
                     &engine, &morfupdate::UpdateEngine::restart, Qt::QueuedConnection);
    QTextStream(stdout) << "morfUpdate agent listening on 127.0.0.1:" << api.port() << '\n';
    const int code = app.exec();
    // Arret propre : on stoppe la boucle du worker et on l'attend avant que
    // `engine` et `worker` ne soient detruits en fin de portee.
    worker.quit();
    worker.wait();
    return code;
}
