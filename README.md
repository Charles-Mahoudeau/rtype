# AYLO Engine

[![Manual](https://img.shields.io/badge/manual-online-blue)](https://charles-mahoudeau.github.io/rtype/)

**AYLO** stands for *Another Yet Lightweight Optimized Engine*.

AYLO is a C++23 game engine built for the Epitech Tek3 R-Type project. It has a Vulkan renderer, Luau scripting,
physics, audio and a replicated client/server network model. It does not contain a game: it runs one.

## Principles

- **Engine is not game.** A game is a folder (`project.luau`, `scripts/`, `assets/`) written only in Luau. The generic
  client and server run it.
- **The Luau API is the product.** A feature is done when a script can use it and the manual documents it.
- **Data over code.** Plugins, backends, window, network and physics are chosen in `project.luau`, not in C++.
- **Everything is a plugin.** Engine features are ECS plugins, each one a shared library that stands alone. Backends
  (platform, render, audio) sit behind contracts. Dependencies point inward.
- **The server is authoritative.** Clients never write replicated data, and bytes from the network never crash the engine.
- **No global state.** State lives in the world.
- **Portable.** Linux, macOS and Windows.

## Build and run

Requires `xmake` and `clang`.

```sh
xmake f -y
xmake build -y
./aylo-runtime [game_dir]            # client
./aylo-runtime --server [game_dir]   # server
./aylo-editor <game_dir>             # editor
```

## Documentation

- [`manual/`](manual/): the manual for game developers (Luau API).
- [`docs/`](docs/): design docs for engine developers.
