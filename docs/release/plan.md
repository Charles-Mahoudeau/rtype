# Engine release: plan

Goal: **a game developer downloads one `.zip` per platform, runs the engine without installing anything, develops a
game with it, and ships the `runtime/` folder with the game.** The same workflow also publishes a rolling nightly.

Based on the code of 2026-11-07 (the generic executable is `aylo-runtime`, the signing tool `aylo-sign`).

## 1. Decisions of this plan

| #  | Decision | Why |
|----|----------|-----|
| D1 | **One zip per platform and architecture**, `aylo-engine-<version>-<os>-<arch>.zip`, holding the runtime, the tools and (when it exists) the editor. No separate editor zip. | One download for a game developer; the `runtime/` folder is the only part a game ships. |
| D2 | **No dependency beyond the OS and the GPU driver.** Third-party libraries are static or bundled; a script fails the build when a binary resolves a library outside `runtime/` (allowlist: the OS and the Vulkan driver). | "Runs without any dependency" is checked, not hoped for. |
| D3 | **No headers or SDK in the zip.** A game is Luau; a native plugin needs the compiler of the engine (`checkLibrary()`), so a separate `-sdk` zip would come later, if asked. | The Luau API is the product. |
| D4 | **SemVer, one version for the engine.** `0.x` until the script API settles; after that a breaking change to what a script sees is a major version. The version is in `xmake.lua`, `--version` and `manifest.json`. | A game developer must know what an upgrade can break. |
| D5 | **The package is made by one xmake task**, run the same locally and in CI. The libraries are real files next to the executable; `plugins/` holds relative links to them on Linux and macOS (copies on Windows), as in the build: a second copy of a library would be loaded twice there. | One recipe, testable on a developer's machine. |
| D6 | **A `v*` tag publishes a draft GitHub Release**; a human reviews and publishes it. Notes come from the Conventional Commits. | Releases are a human decision (`AGENTS.md`). |
| D7 | **Nightly: a rolling `nightly` prerelease**, replaced after every push to `main` whose CI is green (docs-only pushes skipped, a burst of pushes builds the last one). Version `<next>-nightly.<sha>`. | Game developers can test `main`; the stable "latest" release stays stable. |
| D8 | **Signing: free options only.** SHA256SUMS and a GitHub artifact attestation on every zip; ad-hoc `codesign` on macOS; SignPath Foundation for Windows when accepted. Notarization and paid certificates are out of scope. | No certificate budget; the repository is public. |

## 2. Layout of the zip

```
aylo-engine-<version>-<os>-<arch>/
├── runtime/                 what a game ships
│   ├── aylo-runtime(.exe)
│   ├── lib*.so / *.dll / *.dylib
│   ├── plugins/             links to the libraries above (copies on Windows)
│   │   └── backends/
│   └── licenses/
├── editor/                  later, never shipped
├── tools/aylo-sign(.exe)
├── templates/game/          a starter game folder
├── manual/                  offline copy of the manual
├── README.md  LICENSE  VERSION
└── manifest.json            version, library ABI version, compiler, commit, sha256 of every file
```

A game ships `runtime/` and its own `game/` folder.

## 3. Tasks

| Id     | Task | Blocked by | Priority | Size |
|--------|------|------------|----------|------|
| REL-01 | Check script: a build resolves nothing outside its folder | none | P1 | S |
| REL-02 | Self-contained dependencies on the three platforms | REL-01 | P1 | L |
| REL-03 | Engine version: `--version` and `manifest.json` data | none | P1 | S |
| REL-04 | Packaging task: the zip layout | REL-02, REL-03 | P1 | M |
| REL-05 | Smoke test of the zip in a clean directory | REL-04 | P1 | M |
| REL-06 | `release.yml`: tag, matrix, attestation, draft release | REL-05 | P1 | M |
| REL-07 | Nightly prerelease on `main` | REL-06 | P2 | S |
| REL-08 | Sign the binaries: ad-hoc macOS, SignPath on Windows | REL-06 | P3 | M |
| REL-09 | Docs: shipping a game, releasing the engine | REL-04 | P2 | S |

```
REL-01 -> REL-02 --\
                    +-> REL-04 -> REL-05 -> REL-06 -> REL-07
REL-03 ------------/      \                     \-> REL-08
                           \-> REL-09
```

### REL-01: check script

A script (`.github/scripts/check-self-contained.sh`, and a `.ps1` for Windows) that takes a folder and lists every
binary in it whose dynamic dependencies resolve outside it: `ldd` / `readelf -d` on Linux, `otool -L` on macOS,
`dumpbin /dependents` on Windows. An allowlist per platform holds the OS libraries (libc, libm, `libvulkan` where
loaded from the driver, `kernel32`, `libSystem`...). It exits non-zero with the offenders. First run on the current build:
the list of what REL-02 has to fix.

### REL-02: self-contained dependencies

Make every third-party package static or bundled so the check passes. Linux: build on an old glibc baseline (an Ubuntu
22.04 container), static or bundled `libstdc++` and `libgcc`, `libsodium` and the other packages static, Vulkan loaded
so a missing loader gives a clear message. Windows: `/MD` with the VC++ runtime DLLs next to the executable. macOS:
MoltenVK and the Vulkan loader bundled. Rpaths stay relative to the executable. Raise `kLibraryAbiVersion` if a library
entry changes.

### REL-03: engine version

One version in `xmake.lua` (`set_version`), read by `aylo-runtime --version` (version, commit, compiler, library ABI
version) and written to `manifest.json` by REL-04. A nightly stamps `<version>-nightly.<sha>`. Tested.

### REL-04: packaging task

An xmake task (`xmake package-release`) builds in release mode and produces the layout of section 2 in `build/package/`
and the zip next to it: the links of `plugins/` made relative, symbols stripped (debug symbols in a `-symbols` zip),
`manifest.json` with a sha256 per file, third-party licenses in `runtime/licenses/`, `templates/game/` from a small
example game, the offline manual (`uvx zensical build`). Runs the check of REL-01 on `runtime/`.

### REL-05: smoke test

A script that unzips into an empty directory (a bare container on Linux, no toolchain) and runs the checks of a real
user: `aylo-runtime --version`; `aylo-runtime --server --game templates/game` starts, accepts a client connection and
stops cleanly; the check of REL-01 on the unzipped folder; `aylo-sign keygen` works.

### REL-06: `release.yml`

Triggered by a `v*` tag. Matrix Linux, macOS (arm64), Windows: build in release mode, run the tests, package (REL-04),
smoke test (REL-05), `SHA256SUMS`, `actions/attest-build-provenance` on every zip, then a draft GitHub Release with
notes generated from the Conventional Commits and the zips attached. Permissions: `contents: write`,
`id-token: write`, `attestations: write` for this workflow only.

### REL-07: nightly

On every push to `main` (`paths-ignore`: `docs/`, `manual/`, `*.md`), after the CI is green: build, package, smoke test,
attest, and replace the assets of the `nightly` prerelease (the tag moves to the commit). A `concurrency` group with
`cancel-in-progress` builds only the last of a burst. Release notes say it is untested and not for shipping.

### REL-08: signing

macOS: an explicit ad-hoc `codesign -s -` on every binary and library, in the packaging. Windows: apply to the SignPath
Foundation for the repository, then sign the `.exe` and `.dll` in `release.yml` once accepted. Documented limits:
Gatekeeper quarantine on macOS, SmartScreen on Windows.

### REL-09: docs

`manual/`: a page "Shipping your game" (what to copy, how to rename, signing your own build, the macOS quarantine
command, SmartScreen), written with the `write-manual` skill. `docs/release/`: how a release and a nightly are made and
how to cut a version. The README of the zip.
