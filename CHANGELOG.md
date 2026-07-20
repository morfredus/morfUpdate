# Changelog

All notable changes to the project are recorded in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and the project follows [Semantic Versioning](https://semver.org/) (the `VERSION`
file at the repository root).

## [Unreleased]

### Changed
- Updated integration documentation to use canonical production project paths.

## [0.1.0] — 2026-07-13

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
