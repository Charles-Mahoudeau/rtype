# Releasing the engine

How the zips of [`plan.md`](plan.md) are made and published by `.github/workflows/release.yml`. The workflow makes three
things:

| Trigger | Result | Published by |
|---|---|---|
| a tag `vMAJOR.MINOR.PATCH[-prerelease]` | a **draft** GitHub Release: one zip per platform, `SHA256SUMS`, generated notes | a human, in the GitHub interface |
| a push to `main` (not only `docs/`, `manual/` or `.md` files) | the rolling `nightly` prerelease | the workflow |
| a manual run (`workflow_dispatch`) | the zips as workflow artifacts, kept 7 days | nobody: nothing is published |

The workflow never publishes a stable release.

## What each platform job does

On Linux (Ubuntu 22.04, the oldest glibc the zip runs on), macOS (arm64) and Windows:

1. Build in release mode with the tests (`xmake f -y -m release --Tests=y`, `xmake build -y`).
2. `xmake test -v -y`. A failing test stops the platform, and so the release.
3. `xmake package-release -y`: the zip `aylo-engine-<version>-<os>-<arch>.zip` and the `-symbols` zip, in
   `build/package/`.
4. On macOS, check that every binary of the zip has a valid signature ([`signing.md`](signing.md)).
5. The smoke test of the zip (`.github/scripts/smoke-test.sh`, `.ps1` on Windows) in an empty directory.
6. A provenance attestation of each zip (`actions/attest-build-provenance`).

The `publish` job then downloads the three zips, writes `SHA256SUMS` and creates or replaces the release. It is the only
job allowed to write to the repository (`contents: write`).

The build and its tests run again in this workflow rather than being taken from the CI of the same commit: the zip must
come from the tests of the release build, and one path serves the tag, the nightly and the manual run.

### The compiler

On Linux the build uses clang from apt.llvm.org, plus `g++-13` from the toolchain PPA because Ubuntu 22.04 ships
libstdc++ 12, too old for C++23. The CI (`ci.yml`, the build and the lint jobs) installs the same clang through the same
composite action, `.github/actions/setup-clang`: the version is the default of its `version` input, the only place the
number is written, so what the CI checks is what ships. macOS (Apple Clang) and Windows (MSVC) use the compiler of the
`macos-latest` and `windows-latest` images, in both workflows.

## Cut a version

1. On a branch, raise the version in `xmake.lua` (`set_version`). Until the script API settles the version is `0.x`;
   after that a breaking change to what a script sees raises the major version. Merge it by pull request.
2. Once the commit is on `main` and the CI of `main` is green, tag it and push the tag:

   ```sh
   git switch main && git pull
   git tag -a v0.2.0 -m "v0.2.0"
   git push origin v0.2.0
   ```

   The workflow refuses a tag that is not a version, that is not on `main`, or that differs from the version in
   `xmake.lua`.
3. Wait for the workflow. It ends with a **draft** release, `v0.2.0`, holding the three zips and `SHA256SUMS`. A tag with
   a `-` (`v0.2.0-rc.1`) makes a prerelease.
4. Review the draft: the notes (pull requests grouped by label, see `.github/release.yml`; edit them freely), and the
   zips: download one, check it against `SHA256SUMS` and `gh attestation verify <zip> --repo Charles-Mahoudeau/rtype`.
5. Publish the release. This is the only manual step, and it is a human decision.

To redo a failed release: delete the draft and the tag (`git push --delete origin v0.2.0`), fix, tag again.

## The nightly

After every push to `main` that changes more than docs, the workflow builds, tests, packages and smoke tests the commit,
then replaces the content of the `nightly` prerelease:

- The tag `nightly` moves to the commit (force-pushed with the workflow token, which starts no workflow), the old assets
  are deleted and the new zips and `SHA256SUMS` are uploaded. The URL `releases/tag/nightly` never changes.
- The version is `<version>-nightly.<short sha>`: `aylo-runtime --version` and `manifest.json` say which commit a bug
  report ran.
- It is a prerelease and is not marked latest, so the latest release stays the last version. Its notes say it is
  untested and not for shipping.
- A burst of pushes (merged stacked pull requests) builds only the last: a newer push cancels the build of an older one,
  and a publication that finds `main` already moved on does nothing.
- If a repository rule protects tags, the `nightly` tag must be allowed to move for the workflow.

## The contract with the package task

The workflow assumes the package task (REL-04) behaves as follows; change both sides together.

- `xmake package-release` writes `build/package/aylo-engine-<version>-<os>-<arch>.zip` and, next to it, a zip whose name
  ends in `-symbols.zip`.
- `AYLO_VERSION_SUFFIX`, when not empty, is appended to the version with a `-` (`0.2.0` becomes
  `0.2.0-nightly.1a2b3c4`), in the file names, `--version` and `manifest.json`. A release leaves it empty.
- On macOS, `AYLO_CODESIGN_SCRIPT` names a script the task runs on `runtime/` after stripping and before it writes
  `manifest.json` and the zip (`.github/scripts/codesign-macos.sh <runtime folder>`).
- `.github/scripts/smoke-test.sh <zip>` (`smoke-test.ps1` on Windows) is REL-05's script.

## Not done by the workflow

- Publishing a release, and marking it latest.
- Notarizing the macOS binaries, or signing with a paid certificate ([`signing.md`](signing.md)).
- Running it for a pull request: the CI workflow checks those.
