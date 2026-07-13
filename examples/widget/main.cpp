/*
 * morfUpdate — apercu du dialogue de mise a jour
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Affiche l'UpdateDialog avec une release factice pour previsualiser l'UI.
 * (Definir MORFUPDATE_SMOKE=1 le referme automatiquement : verification non interactive.)
 */

#include <QApplication>
#include <QTimer>

#include <morfupdate/UpdateDialog.h>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    morfupdate::ReleaseInfo info;
    info.tag = "v1.5.0";
    info.name = "SiteWatch 1.5.0";
    info.notes =
        "## Nouveautes\n\n"
        "- Nouveau graphique d'evolution\n"
        "- Correction de faux positifs WordPress\n\n"
        "**Merci de votre retour !**";
    info.htmlUrl = QUrl("https://github.com/morfredus/SiteWatch/releases/tag/v1.5.0");
    info.version = morfupdate::Version::parse(info.tag);
    info.valid = true;

    morfupdate::ReleaseAsset asset;
    asset.name = "SiteWatch-1.5.0-win64.zip";
    asset.url = QUrl("https://github.com/morfredus/SiteWatch/releases/download/"
                     "v1.5.0/SiteWatch-1.5.0-win64.zip");
    info.assets.push_back(asset);

    morfupdate::UpdateDialog dlg("SiteWatch", "1.4.2", info);

    if (qEnvironmentVariableIsSet("MORFUPDATE_SMOKE")) {
        // Verification automatique : on ferme apres l'affichage.
        QTimer::singleShot(600, &dlg, &QDialog::accept);
    }

    return dlg.exec();
}
