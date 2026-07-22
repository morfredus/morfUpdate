# morfUpdate

*Lire dans une autre langue : [English](README.md) · **Français** (ce document).*

[![Version](https://img.shields.io/badge/version-0.1.1-blue)](CHANGELOG.md)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.21+-064F8C?logo=cmake)
![Licence](https://img.shields.io/badge/Licence-GPL--3.0--only-blue)

**Bibliothèque C++ commune de détection de mises à jour pour les applications de
bureau** (ComponentHub, SiteWatch, et les outils à venir).

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

- **`GitHubReleaseSource`** — API GitHub Releases (`/releases/latest`, ou la liste
  si on inclut les pré-releases). Jeton optionnel pour dépôts privés / quota.

On peut brancher sa propre source (manifeste JSON auto-hébergé, stub de test…) en
implémentant `IUpdateSource` — voir l'exemple `StubSource` dans
[examples/minimal/main.cpp](examples/minimal/main.cpp).

## Comparaison de versions

`Version` gère `major.minor.patch`, tolère le préfixe `v` (`v1.4.2`) et traite une
pré-release comme antérieure à la stable de même numéro (`1.5.0-beta < 1.5.0`).

## Utilisation

### Le plus simple — vérifier et notifier (UI)

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

### Sans UI — piloter soi-même (cœur seul)

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

## Choix de conception : pas d'auto-update

Le dialogue **ouvre la page de la release / le binaire dans le navigateur** ; le
téléchargement et l'installation restent à la main de l'utilisateur. Un
mécanisme d'installation automatique n'a de sens qu'accompagné d'une
**vérification de signature** ; la porte est laissée ouverte (assets exposés dans
`ReleaseInfo`) pour l'ajouter plus tard proprement.

## Documentation

-   [docs/fr/ARCHITECTURE.md](docs/fr/ARCHITECTURE.md) — les classes (Version, IUpdateSource, GitHubReleaseSource, UpdateChecker, UpdateDialog)
-   [docs/fr/INTEGRATION.md](docs/fr/INTEGRATION.md) — intégrer morfUpdate dans une application
-   [CHANGELOG.md](CHANGELOG.md) — historique des versions
-   [ROADMAP.md](ROADMAP.md) — évolutions prévues
-   [CONTRIBUTING.md](CONTRIBUTING.md) — guide de contribution

> Index : [`docs/fr/`](docs/fr/README.md) (français) · [`docs/en/`](docs/en/README.md) (anglais).

## Licence

Distribué sous la licence [GPL-3.0-only](LICENSE). © 2026 morfredus.
