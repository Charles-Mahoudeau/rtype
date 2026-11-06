# Project settings

`project.luau` is plain Luau, run before anything else, on the server and on every client. It sets values of
`engine.settings`; the engine reads them to decide what to build. Settings are nested tables
(`engine.settings.window.title`), and each part of the engine reads its own section.

**An unknown key is an error**, not ignored, so a typo fails at start with the key's name. A wrong value type is an error
too.

## Flags and arguments

| Name | Meaning |
|---|---|
| `engine.isClient`, `engine.isServer` | Where this file runs |
| `engine.isDebug` | A debug build |
| `engine.args` | The command line flags, typed (see [Getting started](getting-started.md)) |

## Network

| Key | Side | Default | Check |
|---|---|---|---|
| `network.port` | both | `4242` | a whole number from 1 to 65535; `0` also on a server (the system picks a free port) |
| `network.host` | client | `"127.0.0.1"` | must resolve to an address |

## Plugins

Plugins add engine features. A game lists the ones it wants by name:

```luau
engine.settings.plugins = { "physics", "physicsDebug" }
```

| Name | What it adds | Side | Section |
|---|---|---|---|
| `transform` | Positions in a hierarchy | both | none |
| `physics` | 2D physics, see [Physics](physics.md) | both | `physics` |
| `physicsDebug` | Draws the colliders and velocities | client (skipped on a server) | `physicsDebug` |

An unknown name is an error. The time, the schedule and the transform are always there; the plugins that draw, play
sound and read input are added by the client program, so a game does not list them.

### `physics`

Every key is optional and the values are checked on every side, listed or not.

| Key | Meaning |
|---|---|
| `gravity` | `{ x, y }`, world units per second squared, Y down |
| `unitsPerMeter` | world units per meter |
| `maxBodies`, `maxBodyPairs`, `maxContactConstraints`, `tempAllocatorBytes` | positive integers: the simulation's limits |

### `physicsDebug`

| Key | Meaning |
|---|---|
| `enabled` | outlines shown at start (F3 toggles them in game) |
| `drawAll` | outline every collider, not only the entities that ask |
| `outlineThickness`, `velocityScale` | how the outlines and velocity arrows look |

## Window, renderer and audio (client)

```luau
if engine.isClient then
	engine.settings.platform = "glfw"
	engine.settings.renderer = "vulkan"

	engine.settings.window.title = "My game"
	engine.settings.window.size = { 800, 600 }

	engine.settings.audio.backend = "miniaudio"
	engine.settings.audio.volume.music = 0.7

	engine.settings.vulkan.virtualResolution = { 800, 600 }
end
```

| Key | Meaning |
|---|---|
| `platform`, `renderer` | the backends by name (`"glfw"`, `"vulkan"` today); required on a client |
| `window.size`, `window.title`, `window.resizable`, `window.fullscreen` | the window |
| `audio.backend` | `"miniaudio"` or `"null"` (silent) |
| `audio.required` | if the device cannot open: `false` falls back to `"null"`, `true` is an error |
| `audio.maxVoices` | sounds playing at once; the oldest effect is stolen beyond that |
| `audio.volume.master`, `.music`, `.effects` | starting volumes |
| `audio.panDistance` | world units from the listener at which a sound is fully left or right |
| `vulkan.virtualResolution` | `{ width, height }`: the resolution your game is designed for, scaled to the window |
| `vulkan.presentMode` | `"fifo"` (default), `"mailbox"` or `"immediate"` |
| `vulkan.debugging`, `vulkan.layers` | validation, for debugging a GPU problem; `layers` takes `{ "VK_LAYER_KHRONOS_validation" }` |

The `vulkan` section also accepts `engineName`, `extraExtensions`, `minSeverity`, `preferredDeviceType`,
`synchronizationValidation` and `bestPractices`; the `miniaudio` section accepts `sampleRate`, `channels` and
`periodSizeInMilliseconds`. Leave them unset unless you know you need them.

A common pattern is to turn on validation only in debug builds:

```luau
if engine.isDebug then
	engine.settings.vulkan.layers = { "VK_LAYER_KHRONOS_validation" }
	engine.settings.vulkan.debugging = true
end
```
