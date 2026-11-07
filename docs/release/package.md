# The release package

What a game developer downloads, how `xmake package-release` makes it and how it is checked. The plan and the decisions
are in [`plan.md`](plan.md).

## Layout

One zip per platform and architecture, named `aylo-engine-<version>-<os>-<arch>.zip` (`os` is `linux`, `macos` or
`windows`, `arch` `x86_64` or `arm64`), holding one folder of the same name:

```
aylo-engine-<version>-<os>-<arch>/
├── runtime/                 what a game ships
│   ├── aylo-runtime(.exe)
│   ├── lib/                 the libraries that are not plugins: ecs, the contracts, the engine... and the C++ runtime
│   ├── plugins/             the plugin libraries, real files; the engine scans this folder
│   │   └── backends/        the platform, renderer and audio backends, scanned too
│   └── licenses/            THIRD_PARTY_NOTICES.md
├── tools/aylo-sign(.exe)
├── templates/game/          a game that runs, from examples/games/template/game/
├── manual/                  the manual, built for offline reading
├── README.md  LICENSE  VERSION
└── manifest.json
```

The engine looks for `plugins/` and `plugins/backends/` next to the executable, so `runtime/` works wherever it is copied.
The build folder has the same layout (`build/<platform>/<arch>/<mode>/`), so what runs there runs in the zip. No file of
the zip is a symbolic link: every library is one real file, in the one folder that holds it. On Windows, which has no
rpath, the DLLs of `lib/` sit next to the executable instead (there is no `lib/` folder in the zip), which is where
the DLLs of `plugins/` find them. How a binary finds its libraries is in
[`self-contained.md`](self-contained.md#how-the-build-gets-there).
The editor will get its own `editor/` folder, outside `runtime/`, so that shipping the runtime never ships it.

`manifest.json` holds the `version` (a nightly build is `<version>-nightly.<commit>`), the `commit`, the `platform` and
`arch`, the `toolchain`, the `library_abi` and the `sha256` of every other file of the zip. Its version, commit,
toolchain and ABI are read from `aylo-runtime --version`, so it cannot disagree with the binary.

A `-symbols` zip next to it holds the debug information of the binaries (`.debug` files on Linux, `.pdb` on Windows). It
is only made when the build has some: a release build is stripped at link time, so there may be none.

## The version

One place: `set_version` in the root `xmake.lua`. `aylo-runtime --version` prints it:

```
aylo-engine 0.1.0
commit: 772b71bd
toolchain: clang 21.1.0
library ABI: 1
```

A nightly build is configured with `xmake f --nightly=<commit>`, which makes the version `0.1.0-nightly.<commit>`.
`RTYPE_COMMIT` gives the commit to a build made without git. The code is `xmake/modules/engine_version.lua`; the format
is `rtype::core::version::describe` (`src/rtype/core/version/`).

## Making it

```sh
xmake f -y -m release
xmake package-release            # builds aylo-runtime and aylo-sign first (--no-build to skip), then packages
```

The output is in `build/package/`. Options: `--no-build` packages what is already built, `--no-manual` leaves the manual
out (also left out, with a warning, when `uvx` is not installed). The task refuses a build that is not a release build.
It then:

1. copies the runtime, `lib/` and `plugins/` as the build made them, leaving out the libraries made for the tests,
   adds the C++ runtime to `lib/` (Linux, with `patchelf`) and sets the search paths of the executable and the tool to
   the folders of the zip (`$ORIGIN/lib:$ORIGIN/plugins`, and `$ORIGIN/../runtime/lib:$ORIGIN/../runtime/plugins` for
   `tools/aylo-sign`; on Windows its DLLs are copied next to it);
2. adds the licenses, the template game, the manual, the README, the version and the license of the repository (with a
   warning when the repository has none);
3. strips what is not stripped and keeps the debug information apart;
4. runs `.github/scripts/check-self-contained.sh` (`.ps1` on Windows) on `runtime/`, when it exists: a missing script is a
   warning, a failing one stops the package;
5. writes `manifest.json` and makes the zips.

## Checking it

`.github/scripts/smoke-test.sh <zip>` (`.ps1` on Windows) behaves like a user: it unzips into an empty directory and
checks the layout and the hashes of the manifest, the version the runtime reports, the dependencies of `runtime/`, a
server started on `templates/game` and a headless client connecting to it, and `aylo-sign keygen`. Run it on a machine
with no toolchain, such as a bare container, for the answer that counts.

The server's output goes to a file, and a program's output to a file is buffered, so a player joining reaches the log
only when the server stops. The shell script runs the server under `stdbuf -oL` to see it, and so checks the connection
on Linux; on macOS (no `stdbuf`) and on Windows it only checks that the server and the client stay up, and says so. A
log line from the engine on stderr when a client connects would make the check the same everywhere.
