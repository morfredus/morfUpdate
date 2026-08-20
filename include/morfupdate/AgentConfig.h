#pragma once

#include <QHash>
#include <QString>

namespace morfupdate {

// Configuration deliberately contains only stable identities. The HTTP client
// never supplies a repository, URL, service unit or filesystem path.
struct AgentTarget {
    QString project;
    QString service;
    QString repository;
    QString healthUrl;
    QString appDir;
    QString serviceManager;
};

struct AgentConfig {
    quint16 httpPort = 8794;
    QString bindAddress = QStringLiteral("127.0.0.1");
    QString tokenFile;
    // A GitHub fine-grained token with read-only Contents permission. It is
    // deliberately a file reference, never a value returned by this API.
    QString githubTokenFile;
    QHash<QString, AgentTarget> targets;

    static bool load(const QString& path, AgentConfig* config, QString* error);
};

} // namespace morfupdate
