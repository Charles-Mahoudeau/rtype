# Modules

The reference of the repository's modules: what each folder under `src/rtype/` holds, class by class, and the rules
between them. It is the catalogue that used to sit in the root `AGENTS.md`; the root file keeps the one-line map and the
architecture rules, and this file keeps the detail, which changes with every feature. `Plugins.md` has the plugin graph,
`Backend.md` the platforms and renderers, `engine/design.md` the `Engine`.

## Project structure

- `src/rtype/core/` — `core` target, static library (`rtype-core`): what several modules share and that belongs to
  none of them, one folder per feature, included as `core/<feature>/<File>.hpp`, namespace `rtype::core::<feature>`.
  It depends on no other project target and keeps no global state. A class goes in only if two or more unrelated
  modules need it; what only `plugins` needs stays in `plugins`.
    - `core/asset/` (`rtype::core::asset`) — `AssetId`: the hash of an asset's path, the same in every process, which
      components keep to name a texture or a sound. The ECS reflection of it (`kAssetId`) is in
      `plugins/AssetIdReflection.hpp`. `AssetFiles.hpp` (`normalizePath`, `isInsideDirectory`, `resolveAssetPath`, `readTextFile`) and `AssetStore<T>` (the assets
      of a resource by id, with hash-collision checks) are shared by the asset resources of `plugins`, which throw
      `AssetLoadException` (`asset/exceptions/`).
    - `core/hash/` (`rtype::core::hash`) — `NameHash.hpp`: the FNV-1a 64-bit hash (`hashName`, `hashString`,
      `hashInteger`), the same in every compiler and process.
    - `core/config/` (`rtype::core::config`) — `Settings`: the flat configuration every plugin and backend reads
      its own section of, and its `SettingsException` (`config/exceptions/`).
    - `core/library/` (`rtype::core::library`) — `LibraryEntry.hpp`: the C entry point of a library the engine loads
      (`RTYPE_LIBRARY`), its version and toolchain check (`checkLibrary`), and `LibraryRegistrar`, through which libraries
      exchange their registries by name.
    - `core/reload/` (`rtype::core::reload`) — hot reload, free of any module: `Reloadable` (a feature that applies new
      versions of its files), `FileWatcher` (mtime and size polling with a debounce), `ChangeSet`, `ReloadOutcome`
      (applied, failed, needs a rebuild), `ReloadReport` (the log line) and `HotReloader`, which polls and reloads.
    - `core/system/` (`rtype::core::system`) — `CacheDirectory.hpp`, `ExecutablePath.hpp`: where the system keeps a
      user's cache and the running executable.
    - `core/version/` (`rtype::core::version`) — `Version.hpp`: `VersionInfo` and `describe()`, the text of
      `aylo-runtime --version`. The version itself is set once in the root `xmake.lua` (`docs/release/package.md`).
- `src/rtype/ecs/` — `ecs` target, shared library (`rtype-ecs`): the Entity Component System and,
  later, the App and its plugins (`rtype::ecs`). Design in `docs/ecs/design.md`. It depends on `core` only.
  Exported classes and functions are marked `RTYPE_ECS_API`, and the library keeps no
  global mutable state (state lives in the world), so it can also be built static by switching its
  kind and defining `RTYPE_ECS_STATIC` publicly (see `Export.hpp`).
    - Files are grouped in one folder per feature (`entity/`, `type/` (the core shared by every kind of ECS
      data: fields, hooks, descriptions, registry), `component/`, `resource/`, `event/`, `storage/`, `change/`
      (change ticks), `world/`, `command/` (deferred structural changes), `hierarchy/` (`Parent`, `Children`),
      `query/`, `system/`, `call/` (engine calls: named functions plugins give to scripts, and `CallBuffer`, the bytes of
      their arguments and results), `schedule/`, `time/`, `app/` (the App and its plugins)), included as
      `rtype/ecs/<feature>/<File>.hpp`.
    - Exception to the namespace rule: the types users write every day stay in the flat `rtype::ecs`
      namespace whatever their folder (`rtype/ecs/entity/Entity.hpp` declares `rtype::ecs::Entity`).
      Only internal parts get a sub-namespace, named after their folder: `rtype::ecs::storage`,
      `rtype::ecs::detail` (`detail/`), `rtype::ecs::exceptions` (`exceptions/`).
- `src/rtype/network/` — `network` target, static library (`rtype-network`): the transport, which moves bytes
  between machines (sockets, connections, acks, reliability). It knows nothing about the ECS and depends on
  Asio only, as a private package: no public header includes Asio, and only `.cpp` files of `io/` do.
  Exported symbols are marked `RTYPE_NETWORK_API`. Design in `docs/network/design.md`, task plan in
  `docs/network/implementation.md`.
    - Namespaces mirror the folders (no flat-namespace exception as in `ecs`): `rtype::network` for the root
      (`Types.hpp`, `NetworkSettings.hpp`, `RandomSource.hpp`), then one folder and sub-namespace per feature:
      `protocol/` (`rtype::network::protocol`, wire constants, sequences, packets), `bytes/`, `connection/`,
      `io/`, `server/`, `client/`, `exceptions/`. A folder is created only when a file goes in it.
- `src/rtype/replication/` — `replication` target, static library (`rtype-replication`): the only code that
  knows both the ECS and the network; it turns a `World` into messages and back (`rtype::replication`).
  Exported symbols are marked `RTYPE_REPLICATION_API`. Feature folders and sub-namespaces as in `network`:
  `component/`, `event/`, `serialization/`, `manifest/`, `input/`, `snapshot/`, `server/`, `client/`,
  `packet/`, `exceptions/`. `input/InputCalls.hpp` holds its engine calls (`rtype.replication.addInput`, `readInput`),
  `client/ClientCalls.hpp` the client's (`rtype.replication.localClient`), both registered by `ReplicationPlugin`.
- `src/rtype/engine/` — `engine` target, static library (`rtype-engine`): `Engine` runs a game directory (`game/` by
  default, `<dir>`): it reads its `project.luau` through Luau (`scripting/SettingsLoader`, which also exposes
  the command line as `engine.args`), builds the `App` with the program's plugins, `EngineConfig::addPlugins`, and the engine's (transform, replication,
  scripting, server or client) in a fixed order, calls the game's native `EngineConfig::setup`, runs it, and builds it
  again on `requestReload()` or when `project.luau` changes. `reload/` holds `ProjectReloadable` and `ScriptsReloadable`, the engine's hot reload sources (`HotReloader`, `plugins/HotReload.hpp`). `plugin/` holds `SharedLibrary` and `PluginLoader`, which load the plugin libraries, and
  `PluginSignature`. The plugins `project.luau` names under `plugins` are added after the replication plugin. On clients,
  `NetworkInput` fills the `PlayerInput` sent to the server from the `Input` actions. `EngineConfig` holds
  the mode, the command line and these functions; `CommandLine` parses the `--name value` flags. Exported symbols are marked `RTYPE_ENGINE_API`.
  Design in `docs/engine/design.md`.
- `src/rtype/editor/` — `editor` target, static library (`rtype-editor`) and the `aylo-editor` program (`app/main.cpp`: the
  engine in `kStandalone`, `EngineConfig::debug` true, `EditorPlugin` added by `addPlugins`) (`rtype::editor`, macro `RTYPE_EDITOR_API`, empty as the library is static). It
  requires the graphics and capsule plugins and adds no system: it sets the graphics plugin's `PlatformEventFilter` and
  `ToolFrame` resources, and pauses the App's schedules while editing. `EditorPlugin` builds an `EditorState` (the ImGui
  context, the input bridge, the play session, the open scene, what the panels remember) and reads the `editor`
  settings (`scene`, `textEditor`) with `readEditorOptions()`. `gui/` holds `GuiContext`, `GuiInput` (the window's events to ImGui, and which of them the game
  sees), `GameViewport` and `GuiMesh` (ImGui's draw data and textures as a `contract::renderer::Mesh`). `play/` holds
  `PlaySession` (Play, Pause, Step, Stop), `scene/` holds `SceneEdit` (the capsule changes, open, save, new files) and
  `Isolation` (a capsule shown alone, prefab mode), `console/` holds `ConsoleLog` and `OutputCapture` (the program's
  output, for the Console), `shell/` holds `TextEditor` (a file opened in an external editor), all without ImGui. `panels/` holds the Game, Hierarchy, Inspector, Assets, Particles, Console and Stats panels and the menu bar,
  `FieldEditor` (the widgets of a reflected value, assets dropped on fields) and `WorldView`. Dear ImGui (docking
  branch) is a private package: no header of the editor includes it. Design in `docs/Editor.md`.
- `src/rtype/runtime/` — the generic executable `aylo-runtime`: a `main` that runs an `Engine` on `game/` as a client, or
  as the server with `--server`, which loads the plugins and backends from their libraries (the executables link none). The build copies them to the repository root
  (rule `rtype.copy_to_root`). Games whose gameplay is still C++ (`ethan_game(bestoftheworld)`) keep their own `main`
  and give it to the `Engine` as `EngineConfig::setup`. `rtype_duel` is written in Luau (`game/scripts/`); its own `main`
  only picks the side from its first argument.
- `src/rtype/contract/` — the contracts, one shared library per interface (`contract-platform`, `contract-renderer`,
  `contract-audiodevice`, `rtype-contract-<name>`): what the implementations fulfil and the plugins use, with the
  vocabulary they speak, included as `rtype/contract/<name>/<File>.hpp`, namespace `rtype::contract::<name>`, exported
  symbols marked `RTYPE_CONTRACT_<NAME>_API`. A contract knows no ECS and no plugin; the ECS declarations of its types
  live in the plugin that uses them, reopening its namespace. See `docs/Plugins.md` and `docs/Backend.md`.
    - `contract/platform/` — `IPlatform`, `PlatformRegistry` (platforms by name), `WindowConfig`, `event/` (the `Event` variant), `input/` (`Key`,
      `MouseButton`...), `exceptions/`.
    - `contract/renderer/` — `IRenderer` and `RendererRegistry` (renderers by name), then one folder per
      theme. Exception to the namespace rule, as in `ecs`: these folders stay in the flat
      `rtype::contract::renderer` (`rtype/contract/renderer/drawing/Sprite.hpp` declares
      `rtype::contract::renderer::Sprite`); only `math/` and `exceptions/` get a sub-namespace.
        - `drawing/` — `Sprite`, `Shape`, `Text` (a laid-out text: MSDF glyphs), `Texture`, `Color`, `BlendMode`.
        - `view/` — `View` and `CameraView` (what a `View` shows: letterboxed viewport, view-projection, screen and
          world conversions).
        - `particle/` — the particle types: `Particle`, `ParticleSystemDesc`, `ParticleCurve`, `ParticleRandom`.
        - `shaders/` — `CompiledShader` (the plain data of a game shader, with `MaterialDesc`) and the shaders ABI: the
          GLSL every shader is assembled in and the backends' built-in shaders include.
        - `math/`, `exceptions/`.
    - `contract/audiodevice/` — `IAudioDevice`, `AudioTypes`, `AudioDeviceRegistry` (devices by name) and `exceptions/`.
- `src/rtype/plugins/` — the engine, as ECS plugins: one shared library per plugin (`plugin-<name>`,
  `rtype-plugin-<name>`), one folder each, included as `plugins/<name>/<File>.hpp`, namespace `rtype::plugins::<name>`,
  exported symbols marked `RTYPE_PLUGIN_<NAME>_API`. Each plugin keeps its own code and declares its own components,
  resources and events (`RTYPE_ECS_*`). A plugin scripts drive registers its engine calls in `<Name>Calls.hpp`
  (`add<Name>Calls()`, called from its `build()`), named `rtype.<plugin>.<verb>`, with their argument and result structs
  (`RTYPE_ECS_CALL_DATA`). A plugin names what it requires in `getDependencies()`; the program adds it
  first, and no plugin adds another one. See `docs/Plugins.md`.
    - `plugins/graphics/` (`rtype::plugins::graphics`) — `GraphicsPlugin`: starts the platform and the renderer from a
      `BackendRegistry` the executable fills (so it depends on no backend), with the `rtype.Backend` main-thread resource, the
      window events and the PreUpdate / Render systems that poll the window, frame the drawing and draw the `SpriteQueue`
      by layer. Also the `Camera` component (projection and priority, placed by its entity's `Transform`), the
      `TextureAssets` resource (the textures by `AssetId`, queued by `load()` and decoded with stb), the `SpriteQueue`
      resource (what the frame draws) and the `VisibleArea` resource (what the frame shows). Two main-thread resources
      let a tool sit between the window and the game: `PlatformEventFilter` (sees every event first, may change it or
      hide it) and `ToolFrame` (a callback run just before the frame ends, with the platform and the renderer). It
      requires the `InputPlugin` and the `TransformPlugin`.
    - `plugins/input/` (`rtype::plugins::input`) — `InputPlugin`: `Control` (named with text by `controlFromName()`,
      `ControlNames.hpp`, over `contract/platform/input/KeyNames.hpp`), `Input`, `InputAction`, `ButtonStates`,
      the `rtype.Input` resource, fed by a `PreUpdate` system from the `PlatformEvent`s in order, and the typed input
      events (`InputEvents.hpp`) for gameplay code. It knows nothing of the graphics plugin, which requires it.
    - `plugins/capsule/` (`rtype::plugins::capsule`) — `CapsulePlugin`: capsules, entities described once and spawned
      by copy, actors and levels alike. `Capsule` (a flat list of nodes: name, parent, side, the capsule it instances,
      `ComponentValues`), the `Capsules` resource (by the `AssetId` of their path, `define()` again for hot reload),
      `spawnCapsule()`, `loadCapsule()` (switches levels, with the `LoadedCapsule` resource) and `reloadCapsule()`,
      each recorded in a `Commands` buffer (`CapsuleSpawn.hpp`), the `CapsuleInstance` component on
      every spawned root, and the `rtype.capsule.spawn` / `load` / `reload` calls (`CapsuleCalls.hpp`).
      `format/CapsuleText.hpp` (`rtype::plugins::capsule::format`) reads and writes the capsule file format
      (`docs/CapsuleFormat.md`);
      `CapsuleFiles` (the `rtype.CapsuleFiles` resource) reads a game's `.capsule` / `.scene` / `.actor` files from the
      plugin's `finish()` and reads saved files again for `CapsuleReloadable`, the plugin's hot reload source, which
      fills `ReloadedCapsules` for the system that reloads what they spawned.
      `detail/CapsuleSpawner` checks a capsule and records its walk. It requires no plugin. See
      `docs/ecs/features/18-capsules.md`.
    - `plugins/transform/` (`rtype::plugins::transform`) — `TransformPlugin`: the `Transform` component gameplay writes,
      relative to the entity's parent, and the `GlobalTransform` rendering reads, computed from it in `PostUpdate` on
      every side. `TransformMath.hpp` (`combine`, `toLocal`), `WorldPoses` (world poses for what runs before `PostUpdate`,
      such as the physics) and `Reparent.hpp` (`detachInPlace`, `attachInPlace`). The `Engine` adds it, and the plugins that use it
      require it.
    - `plugins/physics/` (`rtype::plugins::physics`) — `PhysicsPlugin`: 2D physics on Jolt (`Collider`, `RigidBody`,
      `Velocity`, the `PhysicsWorld` resource, collision events, scene queries, `moveAndSlide`), simulated on the server
      side, with query-only bodies on clients. Jolt is a private package: only `.cpp` files and `detail/` include it.
      The debug view is in `physics/debug/` (`rtype::plugins::physics::debug`), its own library `plugin-physics-debug`, the engine calls in `PhysicsCalls.hpp`. Design in
      `docs/physics/design.md`.
    - `plugins/audio/` (`rtype::plugins::audio`) — `AudioPlugin`: `AudioRegistry` (starts the device the settings name,
      `"null"` always registered, as `NullAudioDevice`), `AudioConfig`, `AudioVolumes`, the `AudioAssets` resource (sounds by `AssetId`, opened
      and decoded by the device), the `AudioSource` and `AudioListener` components and the `PlaySound` / `PlayMusic` /
      `StopMusic` events, played by client-side PostUpdate systems. `AudioReflection.hpp` reflects the contract's `Bus`. Design in `docs/audio/design.md`.
    - `plugins/http/` (`rtype::plugins::http`) — `HttpPlugin`: HTTPS requests for scripts. `HttpSettings` (the `http`
      section: allowed hosts, limits, secrets), the `HttpClient` resource (libcurl, private in `detail/CurlClient`,
      polled without waiting by `rtype.Http.Poll`), the `HttpResponse` event, the `rtype.http.request` and
      `rtype.http.cancel` calls and the `engine.http.request` helper (`HttpForms.luau`). Design in `docs/Http.md`.
    - `plugins/shape/` (`rtype::plugins::shape`) — `ShapePlugin`: the `ShapeRenderer` component (a `Shape` filled and/or
      outlined at its entity's `GlobalTransform`) and the client Render system that draws them over the sprites, by layer.
      `plugins/GraphicsReflection.hpp` reflects the renderer contract's `BlendMode` and `Rect` as ECS fields. It requires
      the `GraphicsPlugin` and the `TransformPlugin`.
    - `plugins/sprite/` (`rtype::plugins::sprite`) — `SpritePlugin`: the `SpriteRenderer` component, sprite atlases and
      animations; it requires no plugin, so a server adds it alone. `SpriteDrawPlugin` pushes the sprite entities to the
      `SpriteQueue` on a client. `SpriteAtlas`
      (named frames and `SpriteAnimation`s, built by `grid()` or in code), the `AtlasAssets` resource (atlases by
      `AssetId`: Aseprite / TexturePacker JSON files read with nlohmann_json, or images packed at load time with
      stb_rect_pack), the `SpriteAnimator` component and the `AnimationFinished` event. A PostUpdate system on every
      side plays the animators into their `SpriteRenderer`, and a client one hands the atlases' textures to
      `TextureAssets`. nlohmann_json is a private package: only `sprite/detail/` includes it. See `docs/Sprites.md`.
    - `plugins/tilemap/` (`rtype::plugins::tilemap`) — `TilemapPlugin`: levels made in Tiled and parallax backgrounds.
      `Tilemap` (tilesets, tile layers stored in 16x16 chunks, image layers), the `TilemapAssets` resource (maps by
      `AssetId`, Tiled `.tmj` files and external `.tsj` tilesets read with nlohmann_json in `tilemap/detail/`),
      `spawnTilemap()` (one `TilemapLayer` entity per layer) and the `Parallax` component. A client Render system moves
      the `Parallax` entities before the camera, and another pushes the visible chunks' tiles to the
      `SpriteQueue`. It requires the graphics plugin. See `docs/Tilemap.md`.
    - `plugins/text/` (`rtype::plugins::text`) — `TextPlugin`: text from MSDF font atlases made offline by
      msdf-atlas-gen (`assets/fonts/`). `Font` (glyph metrics and kerning), the `FontAssets` resource (fonts by
      `AssetId`, msdf-atlas-gen JSON read with nlohmann_json in `text/detail/`, with the UTF-8 decoder),
      `layoutText()` /
      `measureText()` (`TextLayout.hpp`) and the `TextRenderer` component. A client Render system lays out each text and
      pushes it to the `SpriteQueue` as a `contract::renderer::Text`, drawn by `IRenderer::draw(Text)` (Vulkan batches
      its glyphs in the sprites' order, read by the default shader's glyph variant, `glyph.frag`). It requires the graphics and shader plugins. See `docs/Text.md`.
    - `plugins/shader/` (`rtype::plugins::shader`) — `ShaderPlugin`: game shaders and materials. `ShaderAssets` (shaders
      by `AssetId`, written in the engine's GLSL dialect), `MaterialAssets` (`Material`: a shader and its values),
      the `MaterialInstance` component (an entity's own `instance uniform` values), `ShaderHandles`, which keeps the
      renderer up to date, and `ShaderReloadable`, which reloads the shaders saved in `<game>/assets/shaders`
      for the engine's `HotReloader` (`hotReload`). `ShaderCalls.hpp` holds what `engine.assets.shader` and `material` call. `detail/` holds the parser, the assembler and the compiler. glslang is a private package: only
      `shader/detail/ShaderCompiler.cpp` includes it. It requires the graphics plugin. See `docs/Shaders.md`.
    - `plugins/GlmReflection.hpp` — lets components reflect `glm::vec2`, `vec3` and `vec4` fields.
    - `plugins/AssetIdReflection.hpp` — lets components reflect `core::asset::AssetId` fields (`kAssetId`).
    - `plugins/AssetDirectory.hpp` — the `AssetDirectory` resource the `Engine` inserts: `<game>/assets`, where every
      asset path starts and which it cannot leave (`core::asset::resolveAssetPath()`).
- `src/rtype/tools/sign/` — `aylo-sign`: the keys and signatures of the game plugins (`docs/Plugins.md`).
- `src/rtype/interop/` — header-only targets, one per graphics API (`interop-vulkan`,
  `rtype::interop::vulkan`): the interfaces a platform implements to serve the renderers of that API
  (`IVulkanSurfaceSource`). See `docs/Backend.md`.
- `src/rtype/platform/<library>/` — one target per windowing backend, named `platform-<library>`.
    - `platform/glfw/` — `platform-glfw` target, shared library (`rtype-platform-glfw`): `GlfwPlatform`,
      the GLFW implementation of `IPlatform` and `IVulkanSurfaceSource` (`rtype::platform::glfw`,
      exceptions in `platform/glfw/exceptions/`), registered as `"glfw"` by `registerPlatform()`
      (`Registration.hpp`). The only module that includes GLFW: it translates every GLFW callback and the
      gamepad state into `rtype::contract::platform::Event`s.
- `src/rtype/render/<api>/` — one target per renderer, named `render-<api>`, implementing `contract-renderer`.
    - `render/vulkan/` — `render-vulkan` target, shared library (`rtype-render-vulkan`): `VulkanRenderer`,
      the Vulkan implementation of `IRenderer` (`rtype::render::vulkan`), registered as `"vulkan"` by
      `registerRenderer()` (`Registration.hpp`). Shaders go in `shaders/` and are compiled to SPIR-V by
      the `glsl.spirv` rule defined in its `xmake.lua`.
- `src/rtype/audio/<library>/` — one target per audio backend, named `audio-<library>`.
    - `audio/miniaudio/` — `audio-miniaudio` target, shared library (`rtype-audio-miniaudio`): `MiniaudioDevice`,
      the miniaudio implementation of `IAudioDevice` (`rtype::audio::miniaudio`), configured by `MiniaudioConfig`
      (its `miniaudio` settings section) and registered as `"miniaudio"` by `registerAudio()`
      (`Registration.hpp`). `core/` (`rtype::audio::miniaudio::core`) holds the RAII wrappers of miniaudio's objects
      (`Engine`, `SoundGroup`, one per bus, `Sound`), `VoicePool` the playing voices behind the generational
      `VoiceId`s, `exceptions/` the `MiniaudioException`. miniaudio and stb are private packages: the only module that
      includes `miniaudio.h`, and the executable sees `Registration.hpp` only. The implementation of miniaudio (and of
      stb_vorbis, its OGG decoder) is compiled once, in `vendor/MiniaudioImplementation.cpp`, which clang-tidy skips
      (`vendor/.clang-tidy`).
- `src/rtype/luau/` — `luau` target, static library (`rtype-luau`) for Luau scripting. Failures a correct program can
  hit return
  `Result<T>`; exceptions are only for API misuse (`docs/luau/error-handling.md`). The opt-in sandbox and `Table::freeze()`: `docs/luau/sandbox.md`. The `require` loader (`ModuleLoader`): `docs/luau/modules.md`.
- `src/rtype/scripting/` — `scripting` target, static library (`rtype-scripting`): the Luau bridge, turning what scripts
  declare into ECS data (`rtype::scripting`, exported symbols marked `RTYPE_SCRIPTING_API`). It has its own `Error` /
  `Result`, like `luau`. `component/` holds `ComponentDeclaration` and `registerComponent()`, which computes the
  deterministic layout of a script component and registers it. `view/` (`rtype::scripting::view`) holds what scripts
  touch ECS data with: the `ComponentView` usertype (reads and writes a component's fields in place, checked against a
  `ViewScope` generation), the `Entity` usertype (`registerViewTypes()`) and the field conversions (`readField()`,
  `writeField()`, `fillFromTable()`). `system/` holds `SystemDeclaration`, its reader and `registerSystem()`, which
  turns a script system into a `ScriptCallable` on the App. `call/` holds `callEngine()`, which makes an `ecs` engine
  call from a script's table of argument fields and turns its result into Luau values, for `engine.call` and for
  `ctx.call` (a system's declared `calls`). `setup/` (`rtype::scripting::setup`) holds the setup forms built on those
  calls: each plugin brings its setup forms, in a `<Name>Forms.luau` file; `setup/FormsPrelude.luau` creates
  `engine.assets`, `engine.input` and the helpers of `engine.forms`. `ScriptHost` owns the Luau runtime and the `engine` global
  (constant flags `isClient`, `isServer`, `isDebug`, `engine.ecs.component`, read by `component/DeclarationReader.hpp`,
  and `engine.ecs.system`, read by `system/SystemDeclarationReader.hpp`); a system declares the `resources` and `events` it uses, given to scripts by `ctx.resources` and `ctx.events` (plain-data ones only), through the `ecs` `DynamicResources` and `DynamicEvents` parameters; it knows no `Plugin`. The host and
  the App may be destroyed in either order. `plugin/` holds `ScriptingPlugin`, the `ecs::Plugin` that owns a
  `ScriptHost` and, from `build()`, requires the entry points (`Config::main/server/client`, default `main`: main, then server, then client by side); only they and what they require run, a missing default entry warns on `std::cerr`, a missing explicit one throws. `ScriptHost` seals itself before the first module (setup forms, `Runtime::sandbox()`, deep freeze of `engine`) and owns a `luau::ModuleLoader` (`shareGlobals` off, one global table per file) and sets the owner of declarations to the module's chunk name `scripts/<key>`. `modules/` (`rtype::scripting::modules`) holds the `require` path logic: `ScriptPath` (pure parser of a require string) and `ScriptResolver` (finds the file inside `scripts/`: aliases, `init.luau`, exact case, no escape; the `luau::ModuleLoader` callback; `docs/scripting/plan.md` D10). `exceptions/` holds
  `ScriptLoadException`, thrown when a script cannot be loaded, since `build()` cannot return a `Result`. It links
  `core`, `ecs` and `luau`. Design in `docs/luau/ecs-bridge-plan.md`.
- `tests/<module>/` — GoogleTest unit tests, one folder per module, each with its own `xmake.lua`.
- `examples/<category>/.../<Name>/` — standalone examples, each with its own `xmake.lua`, grouped in
  folders of any depth (e.g. `examples/graphic/render/vulkan/VulkanInstance/`). Every `xmake.lua` under
  `examples/` is an example named after its folder. They are only built when enabled (see Commands).
- `docs/` — design documentation (e.g. `docs/Plugins.md` for the libraries and their dependencies, `docs/ecs/features/17-hierarchy.md` for parents and children,
  `docs/Input.md` for the event and input pipeline, `docs/physics/design.md`
  for the physics, `docs/audio/design.md` for the audio, `docs/Sprites.md` for atlases and animations,
  `docs/Tilemap.md` for tilemaps and parallax, `docs/Text.md` for MSDF text, `docs/Editor.md` for the editor, `docs/Shaders.md` for game shaders,
  `docs/CapsuleFormat.md` for the capsule file format, `docs/engine/design.md` for the `Engine`, its build order and
  reloads; `docs/benchmarks/` for the library and format comparisons behind a choice).
- `.github/` — CI workflow and issue templates.
- `.agents/skills/` — agent skills (`.claude/skills/` symlinks to it).

Each module has its own `xmake.lua`, included from the root `xmake.lua`. Add new sources under the
matching module: `plugin-*`, `contract-*`, `core`, `ecs`, `network`, `replication`, `engine`, `platform-glfw`, `render-vulkan`,
`audio-miniaudio`, `luau` and `aylo-runtime` glob
`**.cpp` in their own folder.

### Dependency rules

Dependencies always point towards the contracts and `ecs`, never away from them:

```
core ←  ecs, contract-*
contract-*  ←  platform-*, render-*, audio-*   (loaded by the engine, built before the executables)
ecs, contract-*  ←  plugin-*                   (loaded by the engine, built before the executables)
luau ←  scripting
network  ←  replication
ecs      ←  replication, scripting
scripting, replication, plugin-transform, plugin-input  ←  engine  ←  executables, examples
```

- `core` depends on no other project target; `ecs` depends on `core` only. `core` is static and keeps no global state,
  so a copy in each binary is harmless.
- `luau`, `network`, `replication`, `scripting` and `engine` are static libraries, linked into the executable, because
  no plugin, backend or contract links them. They may keep state (the Luau VM, sockets), which would be duplicated by a
  shared library linking them: the build refuses it (rule `rtype.static_modules` in the root `xmake.lua`). `ecs` and
  the contracts stay shared: they hold the registries and the type information plugins and backends share, so every
  binary must see one copy. A static module marks its exports with `RTYPE_<LIB>_API`, which expands to nothing
  through the public define `RTYPE_<LIB>_STATIC`.
- `network` depends on Asio only (private): never on the ECS, a plugin or any other project target. `replication`
  depends on `ecs` and `network` and is the only module that knows both; the renderer, the platforms and `luau` never
  include its headers.
- A contract depends on `core` and glm only (`contract-renderer` also on `contract-platform`, as `IRenderer` takes an
  `IPlatform`): never on the ECS or a
  plugin. A type of the ECS never goes in a contract, and what a contract type needs from the ECS is declared in the
  plugin that uses it.
- The platforms, renderers and audio backends link their contract only: neither the ECS nor a plugin. An audio backend
  never depends on a platform or a renderer, nor they on it. miniaudio never leaves `audio/miniaudio/`: no header
  outside it includes `<miniaudio.h>`.
- A plugin depends on `core`, `ecs`, glm, the contracts and plugins it uses, plus its own private packages (Jolt,
  nlohmann_json, glslang, stb: no public header includes them). Never add a dependency on a `platform-*`, `render-*`,
  `audio-*` or `luau` target to it. The plugins form a graph without cycles: `plugin-graphics` knows no plugin that
  draws, and each of them is ordered against it only. A resource several plugins fill is taken as a `ResAppend`.
- Modules exchange contract types only (`rtype::contract::platform::Event`, `rtype::contract::platform::input::Key`...).
  GLFW never leaves `platform/glfw/`: no header outside it includes `<GLFW/glfw3.h>` or uses a `GLFW_*` code.
- A platform module and a renderer module never depend on each other: what a renderer needs from a window that is
  specific to its graphics API goes through the matching `interop-*` target, which both sides depend on (see
  `docs/Backend.md`).
- The executables link no plugin and no backend: the `Engine` loads those `project.luau` names from `plugins/` next to
  the executable and `<game>/plugins/`. Each library exports one entry point (`RTYPE_PLUGIN_LIBRARY`, or `RTYPE_LIBRARY`
  for a backend, which knows no ECS). See `docs/Plugins.md`.
- `engine` links `core`, `ecs`, `luau`, `scripting`, `replication`, and nothing
  depends on it but the executables, the examples and its tests. It links no plugin and no contract: it loads
  them, and a program can still add its own through `EngineConfig::addPlugins`.

