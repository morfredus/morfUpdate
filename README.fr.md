# morfUpdate

*Lire dans une autre langue : [English](README.md) · **Français** (ce document).*

[![Version](https://img.shields.io/badge/version-0.8.3-blue)](CHANGELOG.md)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.21+-064F8C?logo=cmake)
![Licence](https://img.shields.io/badge/Licence-GPL--3.0--only-blue)

**Bibliothèque C++ commune de détection des mises à jour et agent local de mise
à jour vérifiée** pour les applications morfSystem.

Elle compare la version installée à la dernière version publiée et **notifie**
l'utilisateur, sans jamais installer quoi que ce soit en silence.

## Deux couches, séparées

| Couche | Cible CMake | Dépendances | Rôle |
|---|---|---|---|
| **Cœur** | `morfUpdate::morfUpdate` | Qt Core + Network | comparer versions, interroger la source |
| **UI** (optionnelle) | `morfUpdate::Widgets` | + Qt Widgets | dialogue de notification |

Le cœur ne dépend pas de Widgets : utilisable dans un service sans interface, ou
avec une UI maison. La couche Widgets fournit un dialogue prêt à l'emploi.

## Source enfichable

`UpdateChecker` interroge une `IUpdateSource`. Fournie par défaut :

- **`GitHubReleaseSource`** - API GitHub Releases (`/releases/latest`, ou la liste
  si on inclut les pré-releases). Jeton optionnel pour dépôts privés / quota.

On peut brancher sa propre source (manifeste JSON auto-hébergé, stub de test…) en
implémentant `IUpdateSource` - voir l'exemple `StubSource` dans
[examples/minimal/main.cpp](examples/minimal/main.cpp).

## Comparaison de versions

`Version` gère `major.minor.patch`, tolère le préfixe `v` (`v1.4.2`) et traite une
pré-release comme antérieure à la stable de même numéro (`1.5.0-beta < 1.5.0`).

## Utilisation

### Le plus simple - vérifier et notifier (UI)

```cpp
#include <morfupdate/UpdateDialog.h>

morfupdate::morfUpdateConfig cfg;
cfg.owner = "morfredus";
cfg.repo  = "SiteWatch";
cfg.currentVersion = SITEWATCH_VERSION;   // macro déjà définie par CMake

// Au démarrage : silencieux si à jour. Sur un bouton « Rechercher les MAJ » :
// passer false pour afficher aussi « à jour » / les erreurs.
morfupdate::checkAndNotify(this, "SiteWatch", cfg, /*silentIfUpToDate=*/true);
```

### Sans UI - piloter soi-même (cœur seul)

```cpp
#include <morfupdate/UpdateChecker.h>

auto* checker = new morfupdate::UpdateChecker(cfg, this);
connect(checker, &morfupdate::UpdateChecker::updateAvailable, this,
        [](const morfupdate::ReleaseInfo& info) {
            qInfo() << "Nouvelle version :" << info.version.toString();
        });
connect(checker, &morfupdate::UpdateChecker::upToDate, this, [] { /* ... */ });
connect(checker, &morfupdate::UpdateChecker::checkFailed, this,
        [](const QString& e) { qWarning() << e; });
checker->checkForUpdates();   // asynchrone, non bloquant
```

Guide d'intégration détaillé (CMake + où l'appeler) :
**[docs/fr/INTEGRATION.md](docs/fr/INTEGRATION.md)**.

## Compilation

```sh
cmake --preset mingw      # ou linux / linux-arm64
cmake --build --preset mingw
```

En build autonome : `morfupdate_demo` (console) et `morfupdate_widget_demo`
(aperçu du dialogue) sont compilés.

## Essayer

```sh
# Vérification hors-ligne déterministe (source stub)
./build-mingw/examples/minimal/morfupdate_demo

# Vraie vérification GitHub
./build-mingw/examples/minimal/morfupdate_demo github morfredus SiteWatch 0.0.1

# Aperçu du dialogue
./build-mingw/examples/widget/morfupdate_widget_demo
```

## Agent local de mise à jour

`morfupdate-agent` n'écoute que sur `127.0.0.1:8794`. Sur un LAN de confiance, il
accepte les demandes relayées par le morfMonitor local, sans jeton à gérer. Il
n'accepte qu'un projet et une version déclarés, enregistre une opération
asynchrone, puis vérifie le tag GitHub de la release, le manifeste et le SHA-256
avant d'appeler l'installeur de la plateforme. Il n'accepte jamais de commande,
d'URL ni de chemin venus du client. Il sait aussi relancer un service déclaré
(`POST /api/v1/restart`) et installer un projet non compilé (stratégie
`source-bundle`).

Par défaut, il refuse de se mettre à jour **lui-même**. Sous Linux, déclarer une
cible `morfUpdate` avec `"self": true` active une succession en deux temps : il met
de côté le nouveau paquet et celui de retour, passe la main à un **applieur systemd
détaché** hors de son propre cgroup, et c'est son successeur qui valide sa version
et sa santé avant de marquer l'opération réussie ou revenue en arrière. À garder
sur un banc de test tant que ce chemin n'est pas éprouvé sur du vrai matériel.

Le contrat et la configuration requise sont décrits dans
[docs/fr/AGENT-CONTRACT.md](docs/fr/AGENT-CONTRACT.md).

## Côté application de bureau

Le dialogue **ouvre la page de la release dans le navigateur**. L'installation
reste déléguée à l'agent local, qui fait ses vérifications explicites avant de
pouvoir agir.

## Documentation

-   [docs/fr/ARCHITECTURE.md](docs/fr/ARCHITECTURE.md) - les classes (Version, IUpdateSource, GitHubReleaseSource, UpdateChecker, UpdateDialog)
-   [docs/fr/INTEGRATION.md](docs/fr/INTEGRATION.md) - intégrer morfUpdate dans une application
-   [docs/fr/AGENT-CONTRACT.md](docs/fr/AGENT-CONTRACT.md) - contrat de l'agent local de mise à jour
-   [CHANGELOG.md](CHANGELOG.md) - historique des versions
-   [ROADMAP.md](ROADMAP.md) - évolutions prévues
-   [CONTRIBUTING.md](CONTRIBUTING.md) - guide de contribution

> Index : [`docs/fr/`](docs/fr/README.md) (français) · [`docs/en/`](docs/en/README.md) (anglais).

## Licence

Distribué sous la licence [GPL-3.0-only](LICENSE). © 2026 morfredus.
