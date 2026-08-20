# Changelog

All notable changes to the project are recorded in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and the project follows [Semantic Versioning](https://semver.org/) (the `VERSION`
file at the repository root).

## [Unreleased]

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
