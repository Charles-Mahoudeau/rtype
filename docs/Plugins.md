# Plugins

How the engine is split into libraries, and how a program picks the ones it needs. Nothing is mandatory but the base: a
server links no window, and a game can leave out anything it does not use.

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
