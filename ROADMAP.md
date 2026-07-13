# Roadmap

## Done

- **v0.1.0** — semantic-version comparison, pluggable `IUpdateSource` with a
  GitHub Releases default, asynchronous `UpdateChecker`, optional notification
  dialog + `checkAndNotify` helper. Integrated into ComponentHub and SiteWatch.

## Planned

- **Self-hosted manifest source** — a `ManifestSource` implementing
  `IUpdateSource` from a `latest.json` on a web server (alternative to GitHub).
- **Signed auto-update** — optional download + install with signature
  verification (assets are already exposed in `ReleaseInfo`).
- **"Skip this version" / "remind me later"** preferences in the dialog.
- **English translation** of the `docs/fr/` guides under `docs/en/`.
