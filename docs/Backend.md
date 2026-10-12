# Backends

How the engine gets a window and a renderer without knowing which libraries provide them.

The engine never talks to GLFW, Vulkan, SDL or SFML. It talks to two interfaces, **`IPlatform`** (the window and
the input devices) and **`IRenderer`** (2D drawing). Implementations of both are registered by name in a
**`BackendRegistry`**, and a **`Settings`** object, the configuration, picks the pair to start and sets it up.

Runnable examples:

| Example | Shows |
|---|---|
| [`examples/graphic/render/vulkan/VulkanInstance`](../examples/graphic/render/vulkan/VulkanInstance/src/main.cpp) | The engine's backends: `glfw` + `vulkan` |
| [`examples/graphic/platform/sdl/SdlVulkanWindow`](../examples/graphic/platform/sdl/SdlVulkanWindow/src/main.cpp) | Your own platform (`sdl`) with the engine's renderer |
| [`examples/graphic/sfml/SfmlWindow`](../examples/graphic/sfml/SfmlWindow/src/main.cpp) | Your own platform and renderer (`sfml` + `sfml`) |

## Architecture

```mermaid
flowchart BT
    subgraph CORE["engine-core: no window, graphics or scripting library"]
        IP["IPlatform"]
        IR["IRenderer"]
        ST["Settings"]
        BR["BackendRegistry"]
    end

    subgraph INTEROP["interop/: one header-only target per graphics API"]
        IV["IVulkanSurfaceSource<br/>(interop-vulkan)"]
    end

    subgraph PLATFORM["platform/glfw/ (platform-glfw)"]
        GP["GlfwPlatform"]
        RP["registerPlatform(registry)<br/>→ &quot;glfw&quot;"]
    end

    subgraph VULKAN["render/vulkan/ (render-vulkan)"]
        VR["VulkanRenderer"]
        RR["registerRenderer(registry)<br/>→ &quot;vulkan&quot;"]
    end

    subgraph OWN["your own code (e.g. examples)"]
        OB["SdlPlatform, SfmlPlatform, SfmlRenderer...<br/>registry.addPlatform() / addRenderer()"]
    end

    GP -- implements --> IP
    GP -- implements --> IV
    VR -- implements --> IR
    VR -- uses --> IV
    RP -- registers into --> BR
    RR -- registers into --> BR
    OB -- implement / register into --> BR
    BR -- reads --> ST
```

Dependencies keep pointing towards `engine-core` (see `AGENTS.md`): the interfaces, `Settings` and the registry live
there, and know nothing of any library. What a renderer needs from a window that is specific to its graphics API
lives in `interop/`, which both sides depend on: the platform and the renderer never depend on each other. Only the
code that registers the backends (`main.cpp`, an example) names the modules.

| File | Role |
|---|---|
| [`engine/platform/IPlatform.hpp`](../src/rtype/engine/platform/IPlatform.hpp) | The window and its events |
| [`engine/platform/WindowConfig.hpp`](../src/rtype/engine/platform/WindowConfig.hpp) | How the window is created, read from the `window` settings |
| [`engine/graphics/IRenderer.hpp`](../src/rtype/engine/graphics/IRenderer.hpp) | Textures, camera, sprites and rectangles |
| [`engine/config/Settings.hpp`](../src/rtype/engine/config/Settings.hpp) | The configuration |
| [`engine/backend/BackendRegistry.hpp`](../src/rtype/engine/backend/BackendRegistry.hpp) | Backends by name, and the start-up sequence |
| [`interop/vulkan/IVulkanSurfaceSource.hpp`](../src/rtype/interop/vulkan/IVulkanSurfaceSource.hpp) | What a Vulkan renderer needs from a window |
| [`platform/glfw/Registration.hpp`](../src/rtype/platform/glfw/Registration.hpp) | Registers `glfw` |
| [`render/vulkan/Registration.hpp`](../src/rtype/render/vulkan/Registration.hpp) | Registers `vulkan` |

## Usage

```cpp
// 1. Every module registers its backends.
rtype::engine::backend::BackendRegistry registry;
rtype::platform::glfw::registerPlatform(registry);   // "glfw"
rtype::render::vulkan::registerRenderer(registry);  // "vulkan"

// 2. The configuration picks the pair and sets it up.
rtype::engine::config::Settings config;
config.set("platform", "glfw");
config.set("renderer", "vulkan");
config.set("window.title", "R-Type");
config.set("window.size", std::vector<double>{1280, 720});
config.set("vulkan.debugging", true);

// 3. Start them, then only use the interfaces.
const rtype::engine::backend::Backend backend = registry.createBackend(config);
while (backend.platform->isOpen()) {
    for (const auto& event : backend.platform->pollEvents()) { /* ... */ }
    backend.renderer->beginFrame(color);
    backend.renderer->draw(sprite);
    backend.renderer->endFrame();
}
```

`Backend` owns both. Its renderer is destroyed before its platform, since it may hold resources tied to the window
(a Vulkan surface).

## The interfaces

### `IPlatform`

The window, and every native event translated into an engine `Event` (see [`Input.md`](Input.md)): the engine,
`Input` included, never sees the windowing library.

| Function | Role |
|---|---|
| `init(WindowConfig)` | Creates the window. Called once, by the registry. |
| `pollEvents()` | Every event since the previous call. Once per frame. |
| `isOpen()`, `close()` | Window state. |
| `getFramebufferSize()`, `getTitle()`, `setTitle()`, `setSize()` | Window properties. |
| `setCursorLocked()`, `getTime()` | Cursor capture, seconds since `init()` (delta time). |

### `IRenderer`

2D only, at the level every 2D library offers (SFML, SDL, raylib, Vulkan...): how shapes reach the screen is up to
the backend.

| Function | Role |
|---|---|
| `setup(IPlatform&)` | Optional (default: nothing). Sets the platform up before its window exists, and may throw early if the pair does not fit. |
| `init(IPlatform&)` | Attaches to the window. Called once, by the registry, after `IPlatform::init()`. |
| `resize(size)` | Called on every `Resized` event. |
| `createTexture(desc, pixels)`, `destroy(id)` | Textures from RGBA pixels, referred to by `TextureId`. |
| `beginFrame(clearColor)`, `endFrame()` | One frame. |
| `setCamera(camera)`, `draw(Sprite)`, `draw(RectShape)` | Drawing, between `beginFrame()` and `endFrame()`. |

Conventions, the ones of those libraries: world units with X to the right and Y down, angles in radians and
clockwise on screen, shapes drawn in call order (the last one on top). Until `setCamera()` is called, a frame shows
the world in window pixels, origin at the top-left corner.

Resources are **handles**: a `TextureId` is an index and a generation, a plain value to store in sprites and ECS
components. The renderer keeps the real texture in a table, and bumps the generation of a slot when it frees it, so
an id used after `destroy()` is detected instead of reaching the next texture of the slot (see `SfmlRenderer` for an
implementation).

### How a renderer attaches to its platform

Libraries split the window and the rendering differently, so `IRenderer::setup()` and `init()` receive the whole
platform and take what they need. `IPlatform` itself stays free of any graphics API. Two ways exist today:

| Way | Used by | How |
|---|---|---|
| **Interop interface** | `vulkan` with `glfw` or `sdl` | The platform also implements `IVulkanSurfaceSource`; the renderer finds it with a `dynamic_cast`. |
| **Shared window** | `sfml` with `sfml` | The renderer `dynamic_cast`s the platform to the concrete class it is paired with, and draws into its window (`sf::RenderWindow`). A single backend split in two, not a combination to generalize. |

#### Interop interfaces

An interop interface is what a renderer of one graphics API needs from a window, whatever the windowing library. It
is defined **once per API**, in a header-only target of `interop/` that depends only on the headers of that API:

| Target | Interface | Implemented by | Used by |
|---|---|---|---|
| `interop-vulkan` | [`IVulkanSurfaceSource`](../src/rtype/interop/vulkan/IVulkanSurfaceSource.hpp) | `GlfwPlatform`, the SDL example's `SdlPlatform` | `VulkanRenderer` |

```cpp
class GlfwPlatform final : public engine::platform::IPlatform, public interop::vulkan::IVulkanSurfaceSource { ... };
```

| `IVulkanSurfaceSource` | `GlfwPlatform` | Called by `VulkanRenderer` in |
|---|---|---|
| `initLoader(PFN_vkGetInstanceProcAddr)` | `glfwInitVulkanLoader`: GLFW uses the renderer's Vulkan loader, so the process has a single one | `setup()`, before the window exists |
| `getRequiredExtensions()` | `glfwGetRequiredInstanceExtensions` | `init()`, to create the instance |
| `createSurface(VkInstance)` → `VkSurfaceKHR` | `glfwCreateWindowSurface` | `init()`, after the instance |

Why there:

- **Not in `engine-core`**, which would then depend on Vulkan (or pass `void*`).
- **Not in the renderer module**: the platform would depend on the whole renderer (a shared library) for one
  interface.
- **Not in the platform module**: the renderer would depend on GLFW, and could not serve a platform written
  elsewhere (the SDL example).

So it works for any number of platforms and renderers of one API: a new Vulkan-capable platform implements the
interface once, and works with every Vulkan renderer. Another API would get its own target the same way
(`interop-webgpu` with an `IWebGpuSurfaceSource`, `interop-metal` with an `IMetalLayerSource`...), only when a renderer
needs it.

The interfaces are marked `RTYPE_INTEROP_API`, and the engine's (`IPlatform`, `IRenderer`, `IAudio`)
`RTYPE_ENGINE_API`: both give them a default visibility. The renderer, in a shared library, `dynamic_cast`s a
platform created in the executable, which is built with `-fvisibility=hidden`: the cast compares the type
information of the source (`IPlatform`) and of the target (`IVulkanSurfaceSource`) across the two binaries, so both
must be shared. Neither macro exports code (the interfaces are header-only), and both are empty on Windows, where
types are compared by name.

A platform and a renderer that do not fit are reported by the renderer, with a message saying why:
`VulkanRenderer::setup()` throws `UnsupportedFeatureException` before any window opens if the platform does not
implement `IVulkanSurfaceSource`, and `SfmlRenderer::init()` throws if the platform is not an `SfmlPlatform`.

## Start-up sequence

`BackendRegistry::createBackend(config)` runs it, so every pair starts the same way:

```mermaid
sequenceDiagram
    participant Code as main / example
    participant Registry as BackendRegistry
    participant Platform as IPlatform
    participant Interop as IVulkanSurfaceSource
    participant Renderer as IRenderer

    Note over Platform,Interop: the same object (GlfwPlatform),<br/>seen through two interfaces

    Code->>Registry: createBackend(config)
    Registry->>Registry: find the factories named by "platform" and "renderer"
    Registry->>Registry: WindowConfig::fromSettings(config.section("window"))
    Registry->>Platform: platform factory(config.section(platform name))
    Registry->>Renderer: renderer factory(config.section(renderer name))

    Registry->>Renderer: setup(platform)
    Renderer->>Interop: dynamic_cast from IPlatform (throws if not implemented)
    Renderer->>Interop: initLoader(vkGetInstanceProcAddr)

    Registry->>Platform: init(window config)

    Registry->>Renderer: init(platform)
    Renderer->>Interop: getRequiredExtensions()
    Renderer->>Renderer: create the instance (+ layers, messenger)
    Renderer->>Interop: createSurface(instance)

    Registry-->>Code: Backend { platform, renderer }
```

`IPlatform` only receives the generic calls (its factory, `init()`); everything specific to Vulkan goes through
`IVulkanSurfaceSource`, which the renderer gets from the same object with a `dynamic_cast`. With `SfmlRenderer`,
there is no interop step: `setup()` does nothing, and `init()` casts the platform to `SfmlPlatform`.

The order matters: `setup()` is the only moment the renderer can still change how the platform creates its window
(the Vulkan loader must be set before GLFW initializes), and `init()` attaches to a window that already exists.
Everything that can be checked without opening a window (the names, the `window` settings, each backend's settings,
a pair that does not fit) is checked before `IPlatform::init()`.

## Settings

The configuration, independent of where it comes from: filled in code for now, read from a config file later.

It is **flat**: nested tables become dotted keys. The values are booleans, numbers, strings, lists of numbers and
lists of strings.

```
platform        = "glfw"
renderer        = "vulkan"
window.title    = "R-Type"
window.size     = { 1280, 720 }
vulkan.layers   = { "VK_LAYER_KHRONOS_validation" }
vulkan.debugging = true
```

| Function | Role |
|---|---|
| `set(key, value)` | Sets a value. |
| `has(key)` | True if the key has a value. |
| `section(prefix)` | The keys under `prefix.`, without the prefix: `section("window")` turns `window.title` into `title`. |
| `getBool`, `getNumber`, `getString`, `getNumberList`, `getStringList` | Typed reads with a fallback for a missing key; a value of another type throws `SettingsException`. |
| `checkKeys(allowed, context)` | Throws `SettingsException` on the first key not in `allowed`: a typo is reported instead of silently ignored. |

Each part of the engine reads **its own section** and turns it into **its own typed configuration**:

| Section | Read by | Keys |
|---|---|---|
| (root) | `BackendRegistry` | `platform`, `renderer` (names, required) |
| `window` | `WindowConfig::fromSettings()` | `size` ({width, height}), `title`, `resizable`, `fullscreen` |
| `glfw` | the `glfw` factory | none |
| `vulkan` | the `vulkan` factory → `VulkanRenderer::Config` | `engineName`, `layers`, `extraExtensions`, `debugging`, `minSeverity` (`"verbose"`, `"info"`, `"warning"`, `"error"`), `preferredDeviceType` (`"discrete_gpu"` by default, `"integrated_gpu"`, `"virtual_gpu"`, `"cpu"`, `"other"`), `synchronizationValidation` (`true` by default), `bestPractices` (`false` by default), `presentMode` (`"fifo"` by default, `"mailbox"`, `"immediate"`) |

Every key is optional except `platform` and `renderer`; a missing one keeps the default of the typed
configuration. The typed configurations stay usable on their own, for code that builds a backend by hand.

## Errors

Everything is reported by exceptions, before a window opens whenever possible:

| Exception | When |
|---|---|
| `BackendException` | `platform` or `renderer` missing, or not registered (the message lists the registered names). |
| `SettingsException` | An unknown key, a value of the wrong type, an invalid value (`window.size`, `vulkan.minSeverity`, `vulkan.preferredDeviceType`, `vulkan.presentMode`). |
| `UnsupportedFeatureException` | The renderer does not fit the platform (the platform lacks the interop interface it needs), or a function is not implemented yet (`VulkanRenderer` drawing). |
| Others (`std::runtime_error`...) | The backend itself fails: a missing Vulkan layer, a window that cannot be created... |

## Adding a backend

A backend can live in an engine module or in your own code, as the SDL and SFML examples do. In both cases:

1. **Implement the interface.** `class MyPlatform final : public rtype::engine::platform::IPlatform`, or `IRenderer`.
   A platform also implements the interop interfaces of the renderers it serves (`IVulkanSurfaceSource` for
   `vulkan`, with `add_deps("interop-vulkan")`); a renderer overrides `setup()` if it must set the platform up
   before its window exists.
2. **Register it under a name**, with a factory that receives its own section of the settings:

   ```cpp
   registry.addPlatform("sdl", [](const Settings& settings) -> std::unique_ptr<IPlatform> {
       settings.checkKeys({}, "sdl");  // no setting: reject anything
       return std::make_unique<SdlPlatform>();
   });
   ```

   A backend with settings reads them in its factory, into its own typed configuration
   (see [`render/vulkan/Registration.cpp`](../src/rtype/render/vulkan/Registration.cpp)).
3. **Choose it** in the configuration: `config.set("platform", "sdl")`.

In an engine module, a backend gets its own folder and target (`platform/<library>/` → `platform-<library>`,
`render/<api>/` → `render-<api>`), and its registration goes in a `Registration.hpp` with a `registerPlatform()` /
`registerRenderer()` function, exported with the target's `RTYPE_<LIB>_API` macro if it is a shared library.
Self-registration through global variables does not work: the linker drops the unreferenced objects of static
libraries.
