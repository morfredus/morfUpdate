# Documentation de morfUpdate (français)

Bibliothèque C++ commune de détection de mises à jour (GitHub Releases) pour les
applications de bureau, avec un dialogue de notification Qt optionnel, et agent
local qui applique les mises à jour vérifiées des services.

> 🇬🇧 English documentation: [`docs/en/`](../en/README.md) *(index, in progress)*.
> Retour au [README (français)](../../README.fr.md).

## Comprendre et intégrer

| Document | Contenu |
|---|---|
| [Architecture](ARCHITECTURE.md) | Les deux couches (cœur / Widgets) et les classes (Version, IUpdateSource, GitHubReleaseSource, UpdateChecker, UpdateDialog). |
| [Contrat de l'agent](AGENT-CONTRACT.md) | L'agent local de mise à jour : API, validations, relance, stratégies d'installation, auto-mise à jour. |
| [Intégration](INTEGRATION.md) | Brancher morfUpdate dans une application Qt (CMake + code, menu Aide + vérification au démarrage). |

## À la racine du projet

| Document | Contenu |
|---|---|
| [README](../../README.md) | Présentation générale (anglais). |
| [README (français)](../../README.fr.md) | Présentation générale (français). |
| [Journal des versions](../../CHANGELOG.md) | Historique des versions. |
| [Roadmap](../../ROADMAP.md) | Évolutions envisagées. |
| [Contribuer](../../CONTRIBUTING.md) | Guide de contribution. |
