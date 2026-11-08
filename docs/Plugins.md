# Plugins

How the engine is split into libraries, and how a game picks the ones it needs. Nothing is mandatory but the base: a
server loads no window, and a game can leave out anything it does not use.

## The layers

| Layer | Targets | Kind | What it holds |
|---|---|---|---|
| Base | `core` | static | What every module shares: `AssetId`, `Settings`, hashes, system paths |
| Base | `ecs` | shared | The ECS, the `App`, `TimePlugin` |
| Contracts | `contract-platform`, `contract-renderer`, `contract-audiodevice` | shared | The interfaces and their vocabulary. No ECS |
| Implementations | `platform-glfw`, `render-vulkan`, `audio-miniaudio` | shared | What fulfils a contract. They link their contract only, so a window pulls no drawing API |
| Interop | `interop-vulkan` | header-only | What a renderer needs from a window for one graphics API |
| Plugins | `plugin-<name>` | shared | What a game uses: components, resources, events, systems |
| Engine | `engine`, `scripting`, `replication`, `network`, `luau` | shared | Runs a game directory |

`core` is the only static library: it has no global state, so a copy in each binary is harmless.

Names tell the layers apart. A contract is named after its interface (`IRenderer`, `contract-renderer`), a plugin after
the feature a game sees (`graphics`), and an implementation after its domain and its technology (`render-vulkan`).

A contract holds what the GPU, the window or the sound card must understand, and a plugin what the game writes.
`CompiledShader` (SPIR-V and reflection) is in the contract, as the renderer needs it; the shader language, the assets
and the materials are in `plugin-shader`.

## The plugins

| Plugin | Brings | Requires |
|---|---|---|
| `capsule` | `Capsules`, `CapsuleInstance`: actors and levels described once, spawned, loaded and reloaded | |
| `transform` | `Transform`, `GlobalTransform` | |
| `input` | `Input`, actions, `PlatformEvent` and the input events | |
| `graphics` | Starts the window and the renderer, frames, `Camera`, `TextureAssets`, `SpriteQueue` | `input`, `transform` |
| `shader` | The shader language, `Material`, `MaterialInstance` | `graphics` |
| `sprite` | `SpritePlugin`: `SpriteRenderer`, atlases, animations | |
| `sprite` | `SpriteDrawPlugin`: draws the sprites | `SpritePlugin`, `graphics`, `shader`, `transform` |
| `tilemap` | Tiled maps, parallax | `graphics`, `transform` |
| `text` | MSDF fonts, `TextRenderer` | `graphics`, `shader`, `transform` |
| `particle` | Emitters and effects | `graphics`, `shader`, `transform` |
| `audio` | `AudioSource`, `PlaySound`, `AudioVolumes` | `transform` |
| `physics` | Jolt, `Collider`, `RigidBody` | `transform` |
| `physics-debug` | Draws the colliders | `graphics`, `input`, `transform` |
| `ui` | The user interface, laid out by Yoga: nodes, widgets, focus | `graphics`, `input`, `text` |
| `http` | HTTPS requests for scripts: `HttpClient`, `HttpResponse`, `rtype.http.request` (libcurl) | |
| `json` | JSON for scripts: `rtype.json.encode`, `rtype.json.decode` (nlohmann::json), `engine.json.encode`, `decode`, `tryDecode`, `map` | |
| `Editor` | The editor (`rtype-editor`, in `src/rtype/editor/`, not a feature of a game): the game in a panel, its entities and their components, the frame rate, scenes and particles edited. Added by the `aylo-editor` program, see [`Editor.md`](Editor.md) | `graphics`, `capsule` |

A plugin states what it requires in `getDependencies()`. The `App` refuses a plugin whose requirement was not added
first, naming both, so a program adds a plugin after the ones it requires:

```cpp
app.addPlugin(rtype::plugins::transform::TransformPlugin{});
app.addPlugin(rtype::plugins::input::InputPlugin{});
app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{std::move(backends), settings});
app.addPlugin(rtype::plugins::shader::ShaderPlugin{});
app.addPlugin(rtype::plugins::text::TextPlugin{});
```

### What every game has

- `Parent` and `Children` are components of `ecs`, kept by the `World`.
- `TimePlugin`: `App::runFrame()` adds it if it is missing.
- `TransformPlugin`: the `Engine` adds it. It is a plugin like any other: the plugins that use it name it in
  `getDependencies()`, so a program on a bare `App` adds it first.

### Drawing without knowing each other

The plugins that draw push to the `SpriteQueue`, and those that load images to `TextureAssets`, through a
[`ResAppend`](ecs/features/13-scheduler.md#appending-to-a-resource). They need no order between them, so each one is
ordered against the systems of `GraphicsPlugin` only, and a game uses any of them alone.

## Linking

A plugin links the plugins whose code it uses. `physics-debug` links `physics` for `Collider` but does not require the
`PhysicsPlugin`, as it only reads the component. `sprite` links `graphics` and `shader` for `SpriteDrawPlugin`, so a
server that adds `SpritePlugin` loads them, though it opens no window.

Headers of a shared library export what other binaries use: `RTYPE_<TARGET>_API` on classes and functions that have
code in a `.cpp`, `RTYPE_HEADER_ONLY_API` on the interfaces and exceptions that have none. The `detail/` functions the
tests call are exported too.

## Loading

A game names its plugins in `project.luau`, by the name `getName()` gives. The executable links none of them: the
`Engine` loads them from their libraries, so adding or removing a plugin rebuilds nothing.

```lua
engine.settings.plugins = { "rtype.graphics", "rtype.Sprite", "game.Boss" }
```

The plugins a listed plugin requires come with it, with a line on the error output. A name no library registers stops the
start, and the message lists those that exist.

| Folder | What it holds | Loaded by |
|---|---|---|
| `plugins/`, next to the executable | the engine's plugins | every side |
| `plugins/backends/`, next to the executable | the platforms, renderers and audio devices | a client that names a `platform`, a `renderer` or an `audio.backend` |
| `<game>/plugins/` | the game's own plugins | every side |
| `lib/`, next to the executable | the libraries that are not plugins (`ecs`, the contracts, the C++ runtime...) | nothing: it is not scanned |

The build makes this layout (rule `rtype.plugin_library` for `plugins/`, rule `rtype.layout` for `lib/`), and the release
zip holds the same one: every library is one real file, in one folder. A plugin finds the plugins it links beside it
and the other libraries in `lib/` (a relative rpath: `$ORIGIN` and `$ORIGIN/../lib`; a backend, one folder deeper,
`$ORIGIN/../../lib`), so a plugin library is never opened from two places and loaded twice. `plugins/` holds plugins
only: the Engine refuses a library with no entry point, which is why the others are in `lib/`. On Windows, which has no
rpath, the DLLs the executable links sit next to it (the `lib/` ones included), which is where the plugins find them.
A library in `<game>/plugins/` with the file name of an engine library stops the start: a game cannot silently replace
part of the engine.

Libraries stay loaded until the process ends, even through `Engine::requestReload()` and once the `Engine` is gone: they
are opened with `RTLD_NODELETE` (pinned on Windows), so closing one gives its handle back and never unmaps its code. An
exception a plugin threw, caught in `main` after the `Engine` that loaded it was destroyed, still calls that code (its
`what()`). And what they built must outlive
every App. A library and the engine must be built with the same compiler and version: `checkLibrary()` refuses another
one rather than crash later. Raise `kLibraryAbiVersion` when `LibraryEntry` or `LibraryRegistrar` change.

### Entry points

A library exports one C function. A plugin library writes it with `RTYPE_PLUGIN_LIBRARY`:

```cpp
RTYPE_PLUGIN_LIBRARY(registrar) {
    registrar.getPlugins().add("game.Boss", [](const rtype::core::config::Settings& settings) {
        return std::make_unique<BossPlugin>(settings);
    });
}
```

A backend knows no ECS, so it writes `RTYPE_LIBRARY` and fills the registry it finds. The plugin that owns the registry
offers it under a name the contract declares, such as `kRendererRegistryService`, and takes it back when it is built.

### Setup forms

A plugin brings its own forms (`engine.assets.texture`, `engine.input.action`...) in a `<Name>Forms.luau` file, embedded
by `utils.bin2c` and given to `PluginRegistry::addScriptForms()`, by its library and when it is built. A plugin never
links Luau: its forms are source text, written with the helpers of `engine.forms` (`scripting/setup/FormsPrelude.luau`).

Registering them from the library matters on a server: the plugin is not added there, yet its forms exist, and give the
id the asset would have. They test `engine.hasCall()` for that, and use `engine.assetId()`.

## Writing a game's plugin

Three steps, and no file of the engine changes:

1. A folder `<game>/plugins/<name>/`, with the sources, one `RTYPE_PLUGIN_LIBRARY`, and this `xmake.lua`:

   ```lua
   target("game-boss")
       set_kind("shared")
       add_deps("core", "ecs")
       add_files("*.cpp")
       add_rules("rtype.game_plugin")
   ```

2. Its name in `engine.settings.plugins`.
3. In a script, its calls: `engine.call("game.spawnBoss", { level = 3 })`.

The root `xmake.lua` includes the plugins of `game/plugins/`. `examples/games/plugin_demo` is a complete one.

## Signatures

A plugin is native code: it can do anything the player can. A **release** build therefore opens a library of
`<game>/plugins/` only if the project vouches for its author. A debug build checks nothing, as that is where plugins are
written, and the libraries next to the executable are not checked: they are installed with it. Nothing in `project.luau`
turns this off, or a game could disable its own check.

- The project holds the **root key**. Its public half is built into the engine: `xmake f --plugin_root_key=<hex>`.
  Without it, a release build refuses every game plugin. Its secret half never enters the repository.
- A developer holds their own key pair. The root signs their public key: that is their **certificate**.
- The developer signs each build of a library. `<library>.sig` holds their public key, their certificate and the Ed25519
  signature of the library's bytes.

```sh
aylo-sign keygen root                          # once, by the project: root.pub, root.key
aylo-sign keygen dev                           # by the developer: dev.pub, dev.key
aylo-sign certify root.key dev.pub             # by the project: dev.pub.cert
aylo-sign sign dev.key dev.pub libgame-boss.so # by the developer, after each build: libgame-boss.so.sig
```

What a signature does not do:

- It proves who signed, not that the code is honest. A signed plugin can still do anything.
- It covers the library's file, not the libraries it links.
- It cannot revoke a key yet: a leaked developer key stays valid until the root key changes.
- The file is read to check it, then opened again to load it: someone who can write to the folder between the two could
  swap it.
- The engine is open source: anyone can build one without the check. It protects players, it does not lock the engine.
