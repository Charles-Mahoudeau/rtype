# Engine

`rtype::engine::Engine` (`src/rtype/engine/`) runs a game. It reads the game's project settings, builds an `ecs::App`
with the plugins they name, runs it, and can build it again when the project changes. The `Engine` names no backend, and
an executable links none: it loads what `project.luau` names from their libraries ([`../Plugins.md`](../Plugins.md#loading)).

```cpp
rtype::engine::Engine engine{{
    .mode = rtype::ecs::RunMode::kClient,
    .args = {args.begin() + 1, args.end()},
}};
return engine.run();
```

## A game is a directory

```
game/
├── project.luau    settings: window, renderer, audio, network...   (required)
└── scripts/        the Luau files of ScriptingPlugin, loaded in sorted path order (optional)
```

The directory is `EngineConfig::game` (`game` by default), or `--game <directory>` on the command line. A relative path
is resolved against the working directory, then made absolute once, at construction, so a later change of directory
does not move the game. If the directory has no `project.luau`, the constructor throws `exceptions::RuntimeError`.

## The command line

`CommandLine` reads flags only: `--name value`, `--name=value`, or a bare `--name` (`true`). A flag given twice keeps
its last value. Any other argument is an error. The engine itself reads only `--game`. **Every** flag is also given to
`project.luau` as `engine.args`, so the game decides its own command line:

```lua
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end
```

`project.luau` has no standard library (no `tonumber`), so `engine.args` holds typed values (`CommandLine::toValue()`):
`true` and `false` are booleans, a decimal number (`4242`, `-3`, `0.5`, `1e3`) is a number, anything else is a string.

## Executables

| Executable | Mode | Registers | Game code |
|------------|------|-----------|-----------|
| `aylo-runtime --server` (`src/rtype/runtime/`) | `kServer` | nothing (headless) | `game/` only |
| `aylo-runtime` (`src/rtype/runtime/`) | `kClient` | GLFW, Vulkan, miniaudio | `game/` only |
| `rtype_duel` (example) | `server` or `client`, the first argument | as the client, on the client side | its `game/` only (Luau) |
| `ethan_game(bestoftheworld)` (example) | `kStandalone` | as the client | `setup` |

A game written entirely in Luau (SCR-10 in `../scripting/plan.md`) needs no `main` of its own: the generic
runtime runs it (`./aylo-runtime --server --game examples/games/rtype_duel/game`). `rtype_duel` is one: its `main` only picks
the side from its first argument, to show that one binary can run both sides, and registers the client's backends.
`ethan_game` shows the step before: its gameplay is still C++, given to the `Engine` as `EngineConfig::setup`.

The runtime takes `--server` for itself (it is not in `engine.args`); every other flag goes to the game. The build copies
`aylo-runtime` to the repository root (rule `rtype.copy_to_root` in the root `xmake.lua`). The repository root also holds
two committed `sh` wrappers, `r-type_server` (`aylo-runtime --server "$@"`) and `r-type_client`, the names the subject
asks for, and their Windows twins `r-type_server.bat` and `r-type_client.bat` (`.gitattributes` keeps them CRLF). The copy finds the project's shared libraries in the build folder through a relative rpath on Linux and
macOS. On Windows, the rule copies the build folder's DLLs next to them.

## What a build adds, in order

| # | What | When | Why here |
|---|------|------|----------|
| 1 | `project.luau`, read with `isClient`, `isServer`, `isDebug` and `args` | always | everything below reads the settings |
| 2 | the network settings, checked (`network.host`, `network.port`) | always | a wrong address fails before a window opens |
| 3 | `MinimalPlugins`, `TransformPlugin` | always | time, the schedule and the positions, which every game has |
| 4 | `EngineConfig::addPlugins` | if given | plugins a program adds in code rather than through its libraries; a client adds its window and what it draws |
| 5 | `ReplicationPlugin` | network modes (`kClient`, `kServer`) | registers `Owner`, `PlayerInput`, the connection events and `InputLayout`, so the setup can declare its inputs |
| 6 | the plugins named under `plugins`, from their libraries | if listed | the setup and the scripts can use their components and calls |
| 7 | `EngineConfig::setup` | if given | the game's native components, resources, systems and plugins |
| 8 | `ScriptingPlugin`, on `<game>/scripts` | always | the scripts can use what the setup registered |
| 9 | `ServerPlugin` or `ClientPlugin` | network modes | **last**: it reads every replicated component when it is built, the game's and the scripts' |
| 10 | `rtype.engine.networkInput` (`addNetworkInput()`) | `kClient` | fills the `PlayerInput` sent to the server from the `Input` actions named like the `InputLayout` ones, so a game needs no input code of its own |

The network reads the `network` section of the settings and refuses unknown keys:

| Key | Side | Default | Check |
|-----|------|---------|-------|
| `network.port` | both | 4242 | a whole number from 1 to 65535; 0 also on a server (the system picks a free port) |
| `network.host` | client | `"127.0.0.1"` | must resolve to an address |

`addPlugins` runs again on every build, so it creates what the plugins own, such as their registries, each time.

### Plugins

`project.luau` names a game's plugins, so two games differ by data, not by an executable. The `Engine` loads the plugin
libraries (`plugin::PluginLoader`), lets them register, then adds the plugins named under `plugins`, by their real name:

```lua
engine.settings.plugins = { "rtype.Physics", "rtype.PhysicsDebug" }
engine.settings.physics.gravity = { 0, 0 }
engine.settings.physicsDebug.drawAll = true
```

A plugin reads its section when it is built, so a section is only checked if its plugin is listed; an unknown key is then
an error, as in `network`. A server and a client list what they need (`engine.isClient` separates them). See
[`../Plugins.md`](../Plugins.md#loading) for the folders, the entry points and the signatures.

## Assets

Everything a game loads is in `<game>/assets/`, and every path starts there: `engine.assets.texture("ship.png")` reads
`<game>/assets/ship.png`, and the same goes for sounds, fonts, tilemaps, atlases, particle effects and shaders. An asset
is named by this path, the same on every side.

- **The directory is required on a client**, which refuses to start without it, before a window opens. A server can run
  with `project.luau` and `scripts/` alone.
- **A game cannot leave it.** `core::asset::resolveAssetPath()` refuses an absolute path, a path that goes above the
  directory once normalized, and a link that points out of it. The asset is then recorded as failed, like a missing
  file. A `../` that stays inside is fine: a Tiled map in `levels/` names `../backgrounds/a.png`.
- **How the plugins know it.** The `Engine` inserts the `AssetDirectory` resource before it builds them, and each one
  hands it to its asset resource. A program without an `Engine` has none: its paths are relative to the working
  directory and nothing is checked.

## Reloading

Two ways of changing a running game: a full rebuild of the `App`, and an in-place swap of what a script file declares.
Both are driven by the `HotReloader` of `core` (`core/reload/`), which the `Engine` owns.

### The reloader

A `core::reload::Reloadable` names the files it follows (`watched()`) and applies the changed ones (`reload()`), which
returns a `ReloadOutcome`: `kApplied`, `kFailed` (the old version is kept) or `kNeedsRebuild`. The `HotReloader` polls
each reloadable's `FileWatcher` every 250 ms, by modification time and size, and reports a file only after it has the
same stamp on two looks in a row, so a half-written save is not read. A removal is reported at once. It times each
reload, catches an exception as a failure and logs one line:
`[HotReload] scripts: applied in 3.2 ms (modified scripts/ship.luau): system ship.move, component Ship`.
A new hot-reloaded feature is a new `Reloadable` added to the reloader, nothing else.

### Who watches what

- `ProjectReloadable` follows `project.luau`. It runs the file again to check it loads: if not, the reload fails and the
  game keeps its settings; if so, it asks for a rebuild.
- `ScriptsReloadable` follows `scripts/**/*.luau` and calls `ScriptHost::removeScript` / `reloadScript` per file.
  A script is run in a staging area and committed only if the whole file ran; its systems replace the previous ones of
  the same name through `Schedules::replaceSystems`, which restores the old schedule if the new one has a cycle. A change
  that cannot be patched asks for a rebuild: a changed component layout, a new replicated component, a removed component,
  a system that changes phase or whose name another script owns, a deleted file.
- Plugins add their own through the `HotReload` resource (`plugins/HotReload.hpp`, like `AssetDirectory`): the
  `ShaderPlugin` adds a `ShaderReloadable`. The reloader is cleared on each build and the plugins add theirs again.

### In the loop

`run()` calls `App::runFrame()`, reads `App::takeExit()` (an exit wins over a reload), polls the reloader, then rebuilds
if a reload asked for it or `requestReload()` was called (an atomic flag, safe from another thread). Reloads therefore
run between two frames, on the main thread, which is the renderer's.

- **On/off.** `engine.settings.hotReload`, true by default in a debug build. Off: no `HotReload` resource, no polling.
- **Full rebuild.** Everything in the world is lost: entities, resources, the server's clients and the client's
  connection. The old `App` is destroyed before the new one is built, so the window and the socket are released. The
  plugin libraries stay loaded. The engine logs why.
- **Errors.** With hot reload on, a failing rebuild is logged and the engine waits, with no `App`, for the next change of
  the game files to try again. With it off, `run()` throws.
- **Client and server.** Each process watches the same folder and reloads itself. A layout change makes both rebuild; a
  side that differs from the other is refused by the manifest check.

## Dependencies

`engine` is a shared library (`rtype-engine`, `RTYPE_ENGINE_API`). It links `core`, `ecs` and `plugins` publicly,
because `Engine.hpp` and `EngineConfig.hpp` expose the `App`, the `Settings` and the registries. It links `luau`,
`scripting` and `replication` privately. It names no backend: the executables register them, so `engine` never depends
on a `platform-*`, `render-*` or `audio-*` target.
