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
| `engine.isDebug` | A debug build; always `true` in the [editor](editor.md) |
| `engine.args` | The command line flags, typed (see [Getting started](getting-started.md)); `engine.args.editor` is `true` in the [editor](editor.md) |

## Hot reload

| Key | Default | Meaning |
|---|---|---|
| `hotReload` | `true` in a debug build, `false` otherwise | Watch the game's files while it runs, see [Hot reload](hot-reload.md) |

## Network

| Key | Side | Default | Check |
|---|---|---|---|
| `network.port` | both | `4242` | a whole number from 1 to 65535; `0` also on a server (the system picks a free port) |
| `network.host` | client | `"127.0.0.1"` | must resolve to an address |

## Scripts

| Key | Default | Check |
|---|---|---|
| `scripts.main` | `"main"` when none of the three is set | the entry every process runs first |
| `scripts.server` | none | the entry of the server side |
| `scripts.client` | none | the entry of the client side |
| `scripts.aliases.<name>` | none | the folder inside `scripts/` that `@<name>` points to, a string |

An **entry** is a path inside `scripts/`, with no extension and no prefix: `"shared/main"` is
`scripts/shared/main.luau` (or `scripts/shared/main/init.luau`). A process requires `main`, then `server` if it runs
the server side, then `client` if it runs the client side. A standalone process runs both sides, so all three; a shared
module that several entries require runs once. An entry may return nothing.

```luau
engine.settings.scripts.main = "shared/main"
engine.settings.scripts.server = "server/main"
engine.settings.scripts.client = "client/main"
```

- If you set none of the three, `main` is `"main"` (`scripts/main.luau`), which suits a game in one file. When that
  default file does not exist, the game starts with a warning on the error output and runs no script.
- A key you set whose file does not exist stops the start, with a message naming it
  (`Setting 'scripts.server' = 'server/main': ...`). A server does not look for the `client` entry, and a client
  does not look for the `server` one, so a game can ship without the other side's folder.
- A file that no entry requires, directly or not, never runs, and saving it changes nothing while the game runs.
- The same entries work in every mode: you need no `if engine.isServer` in `project.luau` to choose them.

An alias is a short name for a folder of your scripts, usable in `require`:

```lua
engine.settings.scripts.aliases = { shared = "shared" }
```

```lua
-- scripts/server/main.luau
local config = require("@shared/config") -- scripts/shared/config.luau
```

The rules:

- A name is made of `a-z`, `0-9`, `_`, `.` and `-`, and ignores case: `Shared` and `shared` are the same alias, so
  declaring both is an error.
- `scripts` and `self` are reserved: `@scripts` is always the `scripts/` folder, `@self` the folder of the requiring
  file.
- The target is a path relative to `scripts/` and stays inside it: no absolute path, no `..` past the root, no link
  pointing out.
- An invalid alias stops the start with a message naming `scripts.aliases.<name>` and the reason. Any other key under
  `scripts` than `main`, `server`, `client` and `aliases` is an unknown key and an error.

The editor does not read `project.luau`, so mirror each alias in the `.luaurc` next to it (the targets there are
relative to the game folder) or luau-lsp will flag the paths. See [Modules and require](modules.md#editor-setup).

## Plugins

Plugins add engine features. A game lists the ones it wants by name, and the engine loads each from its library:

```luau
engine.settings.plugins = { "rtype.Physics" }
if engine.isClient then
	engine.settings.plugins = { "rtype.Physics", "rtype.Input", "rtype.graphics", "rtype.Sprite", "rtype.SpriteDraw" }
end
```

| Name | What it adds | Side |
|---|---|---|
| `rtype.Input` | Input actions and the keyboard, mouse and gamepad | client |
| `rtype.graphics` | The window and the renderer, cameras and textures | client |
| `rtype.Shader` | Shaders and materials | client |
| `rtype.Sprite` | Sprites, atlases and animations | both |
| `rtype.SpriteDraw` | Draws the sprites | client |
| `rtype.Shape` | Draws filled and outlined shapes | client |
| `rtype.Tilemap` | Tiled maps and parallax backgrounds | client |
| `rtype.Text` | Text from font atlases | client |
| `rtype.Ui`, see [User interface](ui.md) | Menus and HUDs: layout, buttons, sliders, text fields | client |
| `rtype.Editor`, see [Editor](editor.md) | Your scenes edited in a window of panels, and the game played there. Do not list it: `aylo-editor` adds it | both, in one process |
| `rtype.Particle`, see [Particles](particles.md) | Particle emitters and effects | client |
| `rtype.audio` | Sounds, music and the listener | client |
| `rtype.Physics`, see [Physics](physics.md) | 2D physics | both |
| `rtype.PhysicsDebug` | Draws the colliders and velocities | client |
| `rtype.Capsule`, see [Capsules](capsules.md) | Actors and levels from capsule files | both |
| `rtype.Http`, see [HTTP](http.md) | HTTPS requests to the hosts the game names | server |

The time, the schedule and the transform are always there. A plugin that needs another brings it: listing
`rtype.graphics` also loads `rtype.Input`, with a line on the error output, so list what you use and you may leave out
what it requires. A plugin of the game's own (`demo.Greeter`) is named the same way, see
[Native plugins](native-plugins.md). A name no library provides stops the start, and the message lists the names that
exist; the names are case-sensitive.

A plugin reads its section when it is built, so a section is only checked if its plugin is listed; an unknown key in it
is then an error.

### `http`

Every key is optional and the values are checked when `rtype.Http` is listed. The keys are in [HTTP](http.md#settings):
`allowedHosts`, `allowHttp`, `maxConcurrent`, `maxQueued`, `timeout`, `maxResponseBytes` and `secrets`.

### `physics`

Every key is optional and the values are checked when `rtype.Physics` is listed.

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

### `editor`

Read when the game runs in the [editor](editor.md) (`aylo-editor`), ignored otherwise.

| Key | Meaning |
|---|---|
| `scene` | The capsule file, in the assets, the editor opens at start: `"scenes/level.scene"`, or an actor, which opens alone |
| `textEditor` | The command that opens a file from the editor (Visual Studio Code by default): `"subl"`, `"code"` |

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
| `platform`, `renderer` | the backends by name; required on a client. Platforms: `"glfw"`, `"sdl"` (SDL3) or `"raylib"`. Renderers: `"vulkan"` or `"raylib"`. `"raylib"` goes with `"raylib"` only: its window and its drawing come together. `"sdl"` and `"glfw"` both work with `"vulkan"` |
| `window.size`, `window.title`, `window.resizable`, `window.fullscreen` | the window |
| `window.system` | Linux only: `"auto"` (default: Wayland in a Wayland session, X11 otherwise or if Wayland fails), `"x11"` or `"wayland"` |
| `audio.backend` | `"miniaudio"` or `"null"` (silent) |
| `audio.required` | if the device cannot open: `false` falls back to `"null"`, `true` is an error |
| `audio.maxVoices` | sounds playing at once; the oldest effect is stolen beyond that |
| `audio.volume.master`, `.music`, `.effects` | starting volumes |
| `audio.panDistance` | world units from the listener at which a sound is fully left or right |
| `vulkan.virtualResolution` | `{ width, height }`: the resolution your game is designed for, scaled to the window |
| `raylib.virtualResolution` | the same as `vulkan.virtualResolution`, for the renderer `"raylib"` |
| `vulkan.presentMode` | `"fifo"` (default), `"mailbox"` or `"immediate"` |
| `vulkan.debugging`, `vulkan.layers` | validation, for debugging a GPU problem; `layers` takes `{ "VK_LAYER_KHRONOS_validation" }` |

The `vulkan` section also accepts `engineName`, `extraExtensions`, `minSeverity`, `preferredDeviceType`,
`synchronizationValidation` and `bestPractices`; the `miniaudio` section accepts `sampleRate`, `channels` and
`periodSizeInMilliseconds`. Leave them unset unless you know you need them.

On Linux the window opens natively on Wayland when your desktop runs it, so a screen scaled to 150% or 200% is drawn
sharp and at the right size. If a machine has trouble with Wayland (an old graphics driver), let players choose the
window system with a flag of their own:

```luau
if engine.isClient then
	engine.settings.window.system = engine.args.windowSystem or "auto"
end
```

```sh
./aylo-runtime my_game/game --windowSystem x11
```

A common pattern is to turn on validation only in debug builds:

```luau
if engine.isDebug then
	engine.settings.vulkan.layers = { "VK_LAYER_KHRONOS_validation" }
	engine.settings.vulkan.debugging = true
end
```
