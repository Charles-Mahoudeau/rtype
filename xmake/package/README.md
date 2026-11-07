# Aylo engine @VERSION@

A game engine you drive with Luau. This folder is everything you need to make a game and to ship it: nothing else has
to be installed, apart from the graphics driver of the machine (Vulkan) for a game with a window.

## What is in here

| Folder or file | What it is |
|---|---|
| `runtime/` | The engine: `aylo-runtime`, its libraries, its plugins. **This is the folder you ship with your game.** |
| `tools/aylo-sign` | Makes the keys and signatures of the native plugins of a game |
| `templates/game/` | A game that runs: copy it to start yours |
| `manual/` | The manual, to read offline: open `manual/index.html` |
| `manifest.json` | The version of this package, and the hash of every file in it |
| `VERSION`, `LICENSE` | |

## Run a game

From this folder:

```sh
runtime/aylo-runtime --server --game templates/game     # the server, no window
runtime/aylo-runtime --game templates/game              # a client, in a window
runtime/aylo-runtime --version                          # which engine this is
```

(`runtime\aylo-runtime.exe` on Windows.) On macOS, a zip downloaded with a browser is quarantined: run
`xattr -dr com.apple.quarantine .` in this folder once.

## Make a game

A game is a folder with `project.luau`, `scripts/` and `assets/`, written in Luau only. Copy `templates/game/` to
`my_game/game/`, edit it, and run it with `--game my_game/game`. The manual explains the rest.

## Ship it

Copy `runtime/` and your `game/` folder together, and give players the way to start it. Keep `runtime/` as it is: the
engine finds its plugins next to the executable.
