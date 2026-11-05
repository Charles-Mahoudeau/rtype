# Engine

`rtype::engine::Engine` (`src/rtype/engine/`) runs a game. It reads the game's project settings, builds an `ecs::App`
with the engine's plugins, runs it, and can build it again when the project changes. The executables do no wiring by
hand: they register their backends and hand the rest to `Engine`.

```cpp
rtype::engine::Engine engine{{
    .mode = rtype::ecs::RunMode::kClient,
    .args = {args.begin() + 1, args.end()},
    .registerBackends = [](rtype::plugins::backend::BackendRegistry& registry) {
        rtype::platform::glfw::registerPlatform(registry);
        rtype::render::vulkan::registerRenderer(registry);
    },
    .registerAudio = [](rtype::plugins::audio::AudioRegistry& registry) {
        rtype::audio::miniaudio::registerAudio(registry);
    },
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
| `r-type_server` (`src/rtype/server/`) | `kServer` | nothing (headless) | `game/` only |
| `r-type_client` (`src/rtype/client/`) | `kClient` | GLFW, Vulkan, miniaudio | `game/` only |
| `rtype_duel` (example) | `server` or `client`, the first argument | as the client, on the client side | `setupServer` / `setupClient` |
| `ethan_game(bestoftheworld)` (example) | `kStandalone` | as the client | `setup` |

Once a game can be written entirely in Luau (SCR-10 in `../scripting/plan.md`), the two generic executables run it, and
a game no longer needs its own `main`. `rtype_duel` and `ethan_game` show the step in between: their gameplay is still
C++, given to the `Engine` as `EngineConfig::setup`. `rtype_duel` also shows that one binary can run both sides.

The build copies `r-type_client` and `r-type_server` to the repository root (rule `rtype.copy_to_root` in the root
`xmake.lua`). The copies find the project's shared libraries in the build folder through a relative rpath on Linux and
macOS. On Windows, the rule copies the build folder's DLLs next to them.

## What a build adds, in order

| # | What | When | Why here |
|---|------|------|----------|
| 1 | `project.luau`, read with `isClient`, `isServer`, `isDebug` and `args` | always | everything below reads the settings |
| 2 | the network settings, checked (`network.host`, `network.port`) | always | a wrong address fails before a window opens |
| 3 | `MinimalPlugins` | always | time and the schedule |
| 4 | `BackendPlugin`, `AudioPlugin` | client side (`kClient`, `kStandalone`) | from new registries filled by `registerBackends` / `registerAudio` |
| 5 | `ReplicationPlugin` | network modes (`kClient`, `kServer`) | registers `Owner`, `PlayerInput`, the connection events and `InputLayout`, so the setup can declare its inputs |
| 6 | `EngineConfig::setup` | if given | the game's native components, resources, systems and plugins |
| 7 | `ScriptingPlugin`, on `<game>/scripts` | always | the scripts can use what the setup registered |
| 8 | `ServerPlugin` or `ClientPlugin` | network modes | **last**: it reads every replicated component when it is built, the game's and the scripts' |

The network reads the `network` section of the settings and refuses unknown keys:

| Key | Side | Default | Check |
|-----|------|---------|-------|
| `network.port` | both | 4242 | a whole number from 1 to 65535; 0 also on a server (the system picks a free port) |
| `network.host` | client | `"127.0.0.1"` | must resolve to an address |

The registries are filled again on every build: `BackendPlugin` and `AudioPlugin` own theirs, so `EngineConfig` holds
the functions that fill them, never the registries themselves.

## Reloading

`Engine::requestReload()` asks `run()` to read the project again and build a new `App` from it. It is groundwork for
hot reload: nothing calls it yet. A file watcher, a debug key or a Luau call will.

- **When.** Between two frames, never during one: `run()` calls `App::runFrame()`, then reads `App::takeExit()` (an
  exit wins over a reload asked in the same frame), then reloads if asked. `requestReload()` sets an atomic flag, so a
  system or another thread (a file watcher) can call it.
- **What is lost.** Everything in the world: entities, resources, the server's clients and the client's connection.
  The window is closed and opened again, because the old `App` is destroyed before the new one is built, so the
  window and the socket are released before they are needed again. The engine warns on `std::cerr` every time.
- **Errors.** If the new build fails (a script error in `project.luau`, an invalid port...), `run()` throws and the
  engine has no `App` left.

This full reload is what a change to `project.luau` needs: the window, the renderer and the network come from the
settings and cannot change in place. A change to a file of `scripts/` will not need it. The Luau bridge can register a
component again if its `layoutHash` is unchanged, and can replace a system (`../ecs/design.md`, *Hot reload*), so the
world can be kept. That needs the ECS to replace a registered system, which it cannot do yet.

## Dependencies

`engine` is a shared library (`rtype-engine`, `RTYPE_ENGINE_API`). It links `core`, `ecs` and `plugins` publicly,
because `Engine.hpp` and `EngineConfig.hpp` expose the `App`, the `Settings` and the registries. It links `luau`,
`scripting` and `replication` privately. It names no backend: the executables register them, so `engine` never depends
on a `platform-*`, `render-*` or `audio-*` target.
