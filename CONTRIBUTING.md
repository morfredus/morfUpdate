# Contributing to morfUpdate

Thanks for your interest! morfUpdate is a small, shared library compiled into the
apps of the workshop (ComponentHub, SiteWatch, future tools). Keep it **small,
portable and safe**.

## 1. Philosophy

- **Two layers, kept separate.** The core (`morfUpdate::morfUpdate`) never depends
  on Qt Widgets — it must work in a headless service. Only the optional
  `morfUpdate::Widgets` layer pulls Widgets in.
- **Pluggable by design.** New update sources implement `IUpdateSource`; don't
  hard-wire a provider into `UpdateChecker`.
- **Safe by default.** No silent install. Anything that downloads and runs code
  must come with signature verification.
- **Portable.** No platform- or architecture-specific code: identical on Windows,
  Linux x64 and Raspberry Pi (ARM64).

## 2. Building and testing

```sh
cmake --preset mingw      # or linux / linux-arm64
cmake --build --preset mingw
```

Exercise the real paths:

```sh
./build-mingw/examples/minimal/morfupdate_demo                       # offline stub
./build-mingw/examples/minimal/morfupdate_demo github morfredus SiteWatch 0.0.1
./build-mingw/examples/widget/morfupdate_widget_demo                 # dialog preview
```

## 3. Coding conventions

- C++17, Qt idioms (asynchronous signals, parent/child ownership).
- Keep each source file's SPDX header: `SPDX-License-Identifier: GPL-3.0-only`.
- Comments in French are fine (they match the existing code).

## 4. Documentation language

Root documents (`README.md`, `CHANGELOG.md`, `ROADMAP.md`, this file) are in
**English**; a French `README.fr.md` is kept. The in-depth guides live under
`docs/fr/` (French, the reference language). Update the matching document when
you change behavior.

## 5. Reporting bugs / proposing changes

Use **GitHub Issues** to report a problem or suggest an improvement, and open a
pull request for changes.
