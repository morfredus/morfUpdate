# morfUpdate

*Read in another language: **English** (this document) · [Français](README.fr.md).*

[![Version](https://img.shields.io/badge/version-0.1.0-blue)](CHANGELOG.md)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.21+-064F8C?logo=cmake)
![License](https://img.shields.io/badge/License-GPL--3.0--only-blue)

**Shared C++ library for update checking in desktop applications** (ComponentHub,
SiteWatch, and future tools).

It compares the installed version against the latest published version and
**notifies** the user — without ever installing anything silently.

## Two separate layers

| Layer | CMake target | Dependencies | Role |
|---|---|---|---|
| **Core** | `morfUpdate::morfUpdate` | Qt Core + Network | compare versions, query the source |
| **UI** (optional) | `morfUpdate::Widgets` | + Qt Widgets | notification dialog |

The core does not depend on Widgets: usable in a headless service, or with your
own UI. The Widgets layer provides a ready-made dialog.

## Pluggable source

`UpdateChecker` queries an `IUpdateSource`. Provided by default:

- **`GitHubReleaseSource`** — GitHub Releases API (`/releases/latest`, or the list
  when pre-releases are included). Optional token for private repos / rate limit.

You can plug in your own source (self-hosted JSON manifest, test stub…) by
implementing `IUpdateSource` — see the `StubSource` example in
[examples/minimal/main.cpp](examples/minimal/main.cpp).

## Version comparison

`Version` handles `major.minor.patch`, tolerates a `v` prefix (`v1.4.2`) and
treats a pre-release as earlier than the stable of the same number
(`1.5.0-beta < 1.5.0`).

## Usage

### Simplest — check and notify (UI)

```cpp
#include <morfupdate/UpdateDialog.h>

morfupdate::morfUpdateConfig cfg;
cfg.owner = "morfredus";
cfg.repo  = "SiteWatch";
cfg.currentVersion = SITEWATCH_VERSION;   // macro already defined by CMake

// At startup: silent if up to date. On a "Check for updates" menu item:
// pass false to also show "up to date" / errors.
morfupdate::checkAndNotify(this, "SiteWatch", cfg, /*silentIfUpToDate=*/true);
```

### Without UI — drive it yourself (core only)

```cpp
#include <morfupdate/UpdateChecker.h>

auto* checker = new morfupdate::UpdateChecker(cfg, this);
connect(checker, &morfupdate::UpdateChecker::updateAvailable, this,
        [](const morfupdate::ReleaseInfo& info) {
            qInfo() << "New version:" << info.version.toString();
        });
connect(checker, &morfupdate::UpdateChecker::upToDate, this, [] { /* ... */ });
connect(checker, &morfupdate::UpdateChecker::checkFailed, this,
        [](const QString& e) { qWarning() << e; });
checker->checkForUpdates();   // asynchronous, non-blocking
```

Detailed integration guide (CMake + where to call it):
**[docs/fr/INTEGRATION.md](docs/fr/INTEGRATION.md)** *(FR)*.

## Building

```sh
cmake --preset mingw      # or linux / linux-arm64
cmake --build --preset mingw
```

In a standalone build, `morfupdate_demo` (console) and `morfupdate_widget_demo`
(dialog preview) are compiled.

## Try it

```sh
# Deterministic offline check (stub source)
./build-mingw/examples/minimal/morfupdate_demo

# Real GitHub check
./build-mingw/examples/minimal/morfupdate_demo github morfredus SiteWatch 0.0.1

# Dialog preview
./build-mingw/examples/widget/morfupdate_widget_demo
```

## Design choice: no auto-update

The dialog **opens the release page / the binary in the browser**; downloading
and installing stay in the user's hands. An automatic installer only makes sense
with **signature verification**; the door is left open (assets are exposed in
`ReleaseInfo`) to add it properly later.

## Documentation

The in-depth guides are in **French** under [`docs/fr/`](docs/fr/README.md); an
English index is at [`docs/en/`](docs/en/README.md).

| Document | Contents |
|---|---|
| [docs/fr/ARCHITECTURE.md](docs/fr/ARCHITECTURE.md) *(FR)* | The classes (Version, IUpdateSource, GitHubReleaseSource, UpdateChecker, UpdateDialog) |
| [docs/fr/INTEGRATION.md](docs/fr/INTEGRATION.md) *(FR)* | Integrating morfUpdate into an application |
| [CHANGELOG.md](CHANGELOG.md) | Version history |
| [ROADMAP.md](ROADMAP.md) | Planned work |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Contribution guide |

## License

Distributed under the [GPL-3.0-only license](LICENSE). © 2026 morfredus.
