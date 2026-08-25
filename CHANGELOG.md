# Changelog

All notable changes to the project are recorded in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and the project follows [Semantic Versioning](https://semver.org/) (the `VERSION`
file at the repository root).

## [Unreleased]

## [0.4.8] - 2026-08-25

### Ajouté

- **Stratégie d'installation `source-bundle` (projets non compilés, ex.
  morfDashboard).** Le moteur lit `install.type` du manifeste (défaut `package`,
  rétro-compat) au lieu de présumer un binaire : `package` = flux .deb/.zip
  inchangé ; `source-bundle` = archive `.tar.gz` des fichiers applicatifs.
  L'archive est vérifiée (checksum + provenance + VERSION embarquée) puis
  **extraite hors privilège** par l'agent ; l'installation elle-même est un
  **échange atomique** délégué au helper setuid.
- **Helper : verbe `--install-bundle <unpackdir> <service>`.** Échange le
  répertoire applicatif sous `/opt/<service>` (convention, jamais un chemin reçu
  de l'appelant), après sauvegarde `.morfupdate.bak` : arrêt du service, copie de
  l'arborescence extraite, propriétaire repris de l'ancienne install, (re)démarrage
  avec tentatives, et **rollback** (restauration + redémarrage) si le service ne
  revient pas actif. Config, état (`/etc`, `/var/lib`) jamais touchés. Le chemin
  `--install-deb` est inchangé (restart factorisé avec le bundle).

## [0.4.7] - 2026-08-23

### Ajouté

- `GET /status` expose `version` (contrat HTTP). morfMonitor peut afficher la
  version exécutée sans heartbeat beacon.

## [0.4.6] - 2026-08-23

### Corrigé

- Helper après `dpkg` : `daemon-reload`, `reset-failed`, `enable`, puis
  `restart` retenté. Un ancien `prerm` `disable --now` ne laisse plus le service
  éteint (mise à jour de morfMonitor depuis lui-même).

## [0.4.5] - 2026-08-23

### Corrigé

- Après `dpkg`, le contrôle `/healthz` est retenté une minute (20 × 3 s). S'il
  échoue encore, l'opération reste un succès (paquet déjà posé) au lieu d'une
  fenêtre d'échec alors que le service tourne. Le refus du helper inclut
  `QProcess::errorString` quand stderr est vide.

## [0.4.4] - 2026-08-22

### Corrigé

- Helper setuid : `dpkg` et `systemctl` testent l'UID *réel*. Sans `setuid(0)` /
  `setgid(0)` après les contrôles, `dpkg --install` échoue tout de suite alors que
  le même `.deb` s'installe avec `sudo`. Chemins absolus, `DEBIAN_FRONTEND=noninteractive`,
  stderr de dpkg remonté dans le refus.

## [0.4.3] - 2026-08-22

### Changed

- Default `targets` now lists every parc service except morfUpdate itself. An existing install that still has a single test target is not enlarged by `service.py update`; use `config push --force`.

## [0.4.2] - 2026-08-21

### Corrigé

- Helper privilégié : autoriser explicitement l'exécution setuid de Qt (QCoreApplication::setSetuidAllowed) ; sans cela le helper avortait « running setuid, this is a security hole » quand le service non privilégié l'invoquait.
- Copie vendorée de morfdeploy alignée sur 0.17.4 (dossier du helper traversable par le compte de service).

## [0.4.1] - 2026-08-20
### Fixed
- Install the Linux privileged helper during a source-based deployment too, so
  the first verified update does not fail after a successful agent startup.

## [0.4.0] - 2026-08-20
### Changed
- Use anonymous GitHub access for public releases and remove the local Bearer
  token requirement. The agent remains loopback-only and retains the configured
  target allow-list, provenance, checksum and platform checks.

## [0.3.4] - 2026-08-20
### Changed
- Advance the release version so the source tag, production commit and package
  provenance are created from the same revision.

## [0.3.3] - 2026-08-20
### Security
- Store the Windows local API token under ProgramData with a SYSTEM-only ACL shared by the service tasks.

## [0.3.2] - 2026-08-20
### Fixed
- Let a newly installed agent start safely before any update target or GitHub token is configured.

## [0.3.1] - 2026-08-20
### Security
- Separated the Linux privileged package helper from the loopback HTTP agent.

## [0.3.0] - 2026-08-20
### Added
- Added the loopback-only morfUpdate agent and its authenticated asynchronous API.
- Added GitHub release, manifest, tag-commit and SHA-256 verification before installation.
- Added Linux Debian and Windows ZIP installation backends for declared targets.
- Added service packaging metadata for Linux AMD64, Linux ARM64 and Windows x86_64.

## [0.2.0] - 2026-08-20
### Added
- Added the persistent update-operation core used by the local update agent.
- Added strict operation states and a single-active-operation guard.

## [0.1.1] - 2026-07-22
### Changed
- Updated integration documentation to use canonical production project paths.

## [0.1.0] - 2026-07-13

### Added
- First release of **morfUpdate**, the shared update-check library.
- **Core** (`morfUpdate::morfUpdate`, Qt Core + Network): semantic-version
  comparison (`Version`), a pluggable source interface (`IUpdateSource`), a
  default GitHub Releases source (`GitHubReleaseSource`), and an asynchronous,
  non-blocking orchestrator (`UpdateChecker`) emitting `updateAvailable` /
  `upToDate` / `checkFailed`.
- **Optional UI** (`morfUpdate::Widgets`, Qt Widgets): a notification dialog
  (`UpdateDialog`) rendering the Markdown changelog, and a `checkAndNotify(...)`
  helper wiring the checker to the dialog (silent at startup, verbose on demand).
- **No auto-update**: the dialog opens the release page / binary in the browser.
- `morfupdate_demo` (console, with an offline stub and a real GitHub mode) and
  `morfupdate_widget_demo` (dialog preview) examples.
- Verified against the real `morfredus/SiteWatch` repository, and integrated into
  ComponentHub and SiteWatch.
