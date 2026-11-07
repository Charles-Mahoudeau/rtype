# Shipping your game

You make your game with the engine release, and you ship the engine's `runtime/` folder with your game. The player
installs nothing: no compiler, no library, no engine. This page covers the download, what is in it, what to copy when
you ship, and what your players will see the first time they run it.

## Download the engine

Releases are on [GitHub](https://github.com/Charles-Mahoudeau/rtype/releases). Each one holds one `.zip` per platform:

```
aylo-engine-<version>-<os>-<arch>.zip
```

For example `aylo-engine-0.3.0-linux-x86_64.zip`, `aylo-engine-0.3.0-windows-x86_64.zip` or
`aylo-engine-0.3.0-macos-arm64.zip`. Pick the one that matches the machine you develop on.

| Kind | Where | For |
|---|---|---|
| **Stable** | A release named `v<version>` | What you ship. Tested, and it stays available. |
| **Nightly** | The one prerelease named `nightly` | Trying what is coming. It is rebuilt after every change to the engine, it is not tested like a release, and it is replaced each time: never ship it. |

Unzip it anywhere. There is no installer: the folder is the engine.

## What is in the zip

```
aylo-engine-<version>-<os>-<arch>/
  runtime/                what your game ships
    aylo-runtime          the program (aylo-runtime.exe on Windows)
    (libraries)           what the program needs, next to it
    plugins/              the engine's plugins
      backends/           the window, renderer and audio backends
    licenses/             the notices of the libraries inside
  editor/                 planned: the editor. Never shipped.
  tools/aylo-sign         makes the signatures of native plugins
  templates/game/         a small game to start from
  manual/                 this manual, to read offline
  README.md  LICENSE  VERSION
  manifest.json           the version, and the checksum of every file
```

Only `runtime/` goes to your players. Everything else is for you.

## Make a game with it

Copy `templates/game/` next to the zip's folder, rename it, and run it with the program in `runtime/`:

```sh
cp -r aylo-engine-0.3.0-linux-x86_64/templates/game my_game/game

# the server: no window, no graphics
./aylo-engine-0.3.0-linux-x86_64/runtime/aylo-runtime --server --game my_game/game
# a player
./aylo-engine-0.3.0-linux-x86_64/runtime/aylo-runtime --game my_game/game
```

`--game` takes the folder with your `project.luau`, `scripts/` and `assets/`. [Getting started](getting-started.md)
explains the command line, and [Project settings](project-settings.md) the file. With
[hot reload](hot-reload.md) you can change scripts while the game runs.

## Ship your game

A shipped game is two folders side by side: the engine's `runtime/` as it came, and your `game/`. Rename the program to
your game's name; the folder is what matters, so the libraries and plugins are still found.

```
MyGame/
  runtime/
    MyGame              was aylo-runtime
    (libraries, plugins/, licenses/, untouched)
  game/
    project.luau
    scripts/
    assets/
  play.sh               or play.bat on Windows
```

The program looks for `game/` in the folder you start it from, so a small launcher is enough:

```sh
#!/bin/sh
# play.sh
cd "$(dirname "$0")"
exec ./runtime/MyGame --game game "$@"
```

```bat
@echo off
rem play.bat
cd /d "%~dp0"
runtime\MyGame.exe --game game %*
```

Your players run `play.sh`, and a dedicated server runs `./play.sh --server`. Zip the `MyGame/` folder and you have your
release. Check it on a machine that has never seen the engine, or in a clean virtual machine, before you publish it.

!!! warning
    Keep `licenses/` in the folder. The libraries inside the runtime ask for their notices to travel with them.

Leave the files of `runtime/` where they are. A plugin that `project.luau` names but whose file is missing stops the
start with the names that exist; take a file out only if you checked that your game does not name it.

### What the player needs

Nothing to install. The runtime brings its own libraries. A player needs only the operating system and a graphics driver
with Vulkan support, which comes with the driver of any recent graphics card.

## Your players' first run

The runtime is not signed by a company the operating system knows, so the first start can show a warning. Tell your
players, or sign your own build (next section).

**macOS.** A zip downloaded in a browser is marked as coming from the internet, and macOS refuses to open its programs.
This command clears the mark on the unzipped folder:

```sh
xattr -dr com.apple.quarantine MyGame/
```

A download made with `curl` or `gh` is not marked.

**Windows.** SmartScreen shows "Windows protected your PC". Click *More info*, then *Run anyway*.

**Linux.** There is no prompt. If the zip tool dropped the right to run, restore it with
`chmod +x runtime/MyGame play.sh`.

## Sign your own build

The engine's zip carries free, basic protection (see below). A shipped game looks more trustworthy, and gives your
players no prompt, when you sign it yourself. This is done with the usual tools of each platform, on your files, after
you renamed the program:

- **Windows:** sign `MyGame.exe` and the `.dll` files with a code-signing certificate and `signtool`. Certificates are
  sold by certificate authorities. Some programs sign open-source projects for free.
- **macOS:** sign the program and every library with `codesign`, then have Apple notarize the folder. This needs an
  Apple Developer account. Without it, the quarantine command above is the way.
- **Linux:** nothing is needed.

## Check what you downloaded

Every release comes with `SHA256SUMS`. Download it next to the zips and compare:

```sh
sha256sum --check --ignore-missing SHA256SUMS      # Linux
shasum -a 256 --check --ignore-missing SHA256SUMS  # macOS
```

```powershell
Get-FileHash aylo-engine-0.3.0-windows-x86_64.zip  # Windows: compare with the line of SHA256SUMS
```

Every zip also has a GitHub **attestation**: a statement signed by GitHub that the zip was built by the project's own
workflow from a given commit. With the [GitHub CLI](https://cli.github.com):

```sh
gh attestation verify aylo-engine-0.3.0-linux-x86_64.zip --repo Charles-Mahoudeau/rtype
```

It prints the commit and the workflow, or fails if the file is not what the project built.

## Native plugins

A game written in Luau needs nothing more. If your game ships a [native plugin](native-plugins.md), the release build of
the engine opens it only if it comes with a signature, made with `tools/aylo-sign`, from a developer the project vouches
for. Ask the project for a certificate for your key, and sign each build of your library. See
[Native plugins](native-plugins.md#what-the-engine-checks).

## Update the engine

Engine versions follow [semantic versioning](https://semver.org): `major.minor.patch`.

| Version | Meaning |
|---|---|
| `0.x.y` | The script API is still settling. **A minor version can break your scripts.** Read the notes of a release before you update, and ship the version you tested. |
| `1.0.0` and later | A patch fixes bugs, a minor version adds features and keeps your scripts working, and only a major version can break them. |

To update, unzip the new release and replace the **whole** `runtime/` folder of your game with the new one. Do not mix
files from two versions: the libraries and plugins of a release are made for each other. Then run your game and play it
once through before you ship.

## See also

- [Getting started](getting-started.md): the command line and the folder of a game.
- [Project settings](project-settings.md): the plugins and backends your game chooses, which must exist in `runtime/`.
- [Native plugins](native-plugins.md): the C++ plugins a game can ship, and their signatures.
