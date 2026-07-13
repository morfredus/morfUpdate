/*
 * morfUpdate — exemple de demonstration
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Deux modes :
 *   (defaut)                  : source STUB hors-ligne, resultat deterministe.
 *   github <owner> <repo> [v] : vraie verification via l'API GitHub Releases.
 */

#include <QCoreApplication>
#include <QTimer>

#include <morfupdate/UpdateChecker.h>
#include <morfupdate/IUpdateSource.h>
#include <morfupdate/GitHubReleaseSource.h>

// Source factice : renvoie une release canned, sans reseau.
class StubSource : public morfupdate::IUpdateSource {
    Q_OBJECT
public:
    using IUpdateSource::IUpdateSource;
    void fetchLatest(bool) override {
        QTimer::singleShot(0, this, [this]() {
            morfupdate::ReleaseInfo info;
            info.tag = "v9.9.9";
            info.name = "Version de demonstration";
            info.notes = "## Nouveautes\n\n- Ceci est un changelog factice.";
            info.htmlUrl = QUrl("https://example.invalid/releases/9.9.9");
            info.version = morfupdate::Version::parse(info.tag);
            info.valid = true;
            emit latestReady(info);
        });
    }
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();

    morfupdate::morfUpdateConfig cfg;
    morfupdate::UpdateChecker* checker = nullptr;

    if (args.size() >= 4 && args[1] == "github") {
        cfg.owner = args[2];
        cfg.repo = args[3];
        cfg.currentVersion = args.size() >= 5 ? args[4] : "0.0.1";
        qInfo("Verification GitHub : %s/%s (version installee %s)...",
              qUtf8Printable(cfg.owner), qUtf8Printable(cfg.repo),
              qUtf8Printable(cfg.currentVersion));
        checker = new morfupdate::UpdateChecker(cfg, &app);
    } else {
        cfg.currentVersion = "1.0.0";
        qInfo("Mode STUB (hors-ligne). Version installee %s vs 9.9.9 simulee...",
              qUtf8Printable(cfg.currentVersion));
        checker = new morfupdate::UpdateChecker(cfg, new StubSource(&app), &app);
    }

    QObject::connect(checker, &morfupdate::UpdateChecker::updateAvailable, &app,
                     [](const morfupdate::ReleaseInfo& info) {
                         qInfo("MISE A JOUR DISPONIBLE : %s (%s)",
                               qUtf8Printable(info.version.toString()),
                               qUtf8Printable(info.name));
                         qInfo("  Page : %s", qUtf8Printable(info.htmlUrl.toString()));
                         QCoreApplication::quit();
                     });
    QObject::connect(checker, &morfupdate::UpdateChecker::upToDate, &app,
                     [](const morfupdate::ReleaseInfo& latest) {
                         qInfo("DEJA A JOUR (derniere publiee : %s)",
                               qUtf8Printable(latest.version.toString()));
                         QCoreApplication::quit();
                     });
    QObject::connect(checker, &morfupdate::UpdateChecker::checkFailed, &app,
                     [](const QString& err) {
                         qWarning("ECHEC : %s", qUtf8Printable(err));
                         QCoreApplication::quit();
                     });

    checker->checkForUpdates();
    return app.exec();
}

#include "main.moc"
