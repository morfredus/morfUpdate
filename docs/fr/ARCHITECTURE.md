# Architecture — morfUpdate

Retour à l'[index de la documentation](README.md).

---

morfUpdate est découpé en **deux couches** : un cœur sans interface (Qt Core +
Network) et une couche UI optionnelle (Qt Widgets). Le cœur ne connaît pas les
Widgets — on peut l'utiliser dans un service, ou avec sa propre UI.

```
UI (optionnelle)   morfUpdate::Widgets
  UpdateDialog + checkAndNotify(...)        [Qt Widgets]
        │ écoute les signaux
        ▼
Cœur               morfUpdate::morfUpdate   [Qt Core + Network]
  UpdateChecker  ──interroge──►  IUpdateSource ──► GitHubReleaseSource
       │ compare avec                                (API GitHub Releases)
       ▼
  Version (semver)  /  ReleaseInfo (résultat)
```

## Le cœur (`morfUpdate::morfUpdate`)

### `Version`

Version sémantique simplifiée : `major.minor.patch[-prerelease]`. Tolère le
préfixe `v` (`v1.4.2`). Une pré-release est **antérieure** à la stable de même
numéro (`1.5.0-beta < 1.5.0`). Fournit `compare()` et les opérateurs.

### `ReleaseInfo` / `ReleaseAsset`

Description d'une version publiée, **indépendante de la source** : `version`,
`tag`, `name`, `notes` (Markdown), `htmlUrl`, `publishedAt`, `prerelease`, et la
liste des `assets` téléchargeables. C'est ce que `UpdateChecker` compare à la
version courante.

### `morfUpdateConfig`

Paramètres d'une vérification : `owner`, `repo`, `currentVersion`,
`includePrereleases`, `githubToken` (optionnel).

### `IUpdateSource` (interface enfichable)

Source **asynchrone** de la dernière version. `fetchLatest(includePrereleases)`
émet ensuite `latestReady(ReleaseInfo)` ou `failed(QString)`. On peut brancher
une source maison (manifeste auto-hébergé, stub de test).

- **`GitHubReleaseSource`** (défaut) : interroge l'API GitHub Releases via
  `QNetworkAccessManager` (`/releases/latest`, ou la liste si les pré-releases
  sont incluses). En-tête `User-Agent` obligatoire, jeton `Authorization`
  optionnel.

### `UpdateChecker`

Orchestrateur. `checkForUpdates()` demande à la source la dernière version, la
compare à `currentVersion`, puis émet **l'un** de :

- `updateAvailable(ReleaseInfo)` — une version plus récente existe ;
- `upToDate(ReleaseInfo)` — déjà à jour ;
- `checkFailed(QString)` — réseau / parsing / dépôt introuvable.

Asynchrone et **non bloquant** : l'UI reste réactive.

## La couche UI (`morfUpdate::Widgets`)

### `UpdateDialog`

Boîte de dialogue « nouvelle version disponible » : version courante vs nouvelle,
changelog (Markdown rendu par `QTextBrowser`), boutons **Voir la release** /
**Télécharger** / **Plus tard**. Les boutons **ouvrent le navigateur**
(`QDesktopServices`) — pas d'installation automatique.

### `checkAndNotify(...)`

Fonction de commodité : câble un `UpdateChecker` et, si une mise à jour existe,
affiche l'`UpdateDialog`. Le paramètre `silentIfUpToDate` distingue la
vérification **au démarrage** (silencieuse) de la vérification **manuelle**
(affiche aussi « à jour » / les erreurs).

## Choix de conception : pas d'auto-update

Le téléchargement et l'installation restent à la main de l'utilisateur. Un
installeur automatique n'a de sens qu'avec **vérification de signature** ; les
`assets` sont déjà exposés dans `ReleaseInfo` pour l'ajouter proprement plus
tard.
