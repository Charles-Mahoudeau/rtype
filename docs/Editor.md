# Editor

The editor is the program `aylo-editor`: a static library, `rtype-editor` (`src/rtype/editor/`, target `editor`), whose
plugin `rtype.Editor` the program adds to the engine, and a `main` (`editor/app/main.cpp`). It runs the game inside the "Game" panel of a window of docked panels made with Dear ImGui (the docking
branch), and edits its scenes the way Unity does: while editing, the gameplay is paused and the panels change the
scene's capsule, saved to its file; while playing, they change the running world, and Stop puts the scene back. For what
a game developer does with it, see `manual/pages/editor.md`.

```sh
./aylo-editor examples/games/rtype_duel/game
```

```lua
engine.settings.editor.scene = "scenes/level.scene"  -- optional: the scene opened at start
```

## Where it lives, and why

A program of its own, not a feature of a game and not a mode of the runtime. Game developers make their game in
`aylo-editor` and ship it with `aylo-runtime`, which does not know the editor:

- **Not in `plugins/`.** A game never ships it. It has its own folder, namespace `rtype::editor` and export macro
  `RTYPE_EDITOR_API` (empty: the library is static, `RTYPE_EDITOR_STATIC`).
- **A program, like the runtime.** `aylo-editor` links `engine` and `editor` (both static), and the Engine loads the
  plugins and backends from their libraries as in `aylo-runtime`. The editor library links the plugin libraries it uses
  (graphics, capsule, particle) for their types, which the Engine opens again by name: one copy, as the same file.
- **Always a debug run.** `EngineConfig::debug` is true, so `engine.isDebug` is true and hot reload defaults on, even in
  a release build. The check of the game plugins' signatures still follows the build type.
- **Both sides by default.** `RunMode::kStandalone` (both sides in one world, no network: a client alone would not run
  the server's gameplay). "Play online" rebuilds it as `RunMode::kClient` against a child server (see
  [Playing online](#playing-online)). `--server` is refused. The program adds `rtype.Editor` through `EngineConfig::addPlugins`, reads
  its `editor` settings with `readEditorOptions()`, and gives `engine.args.editor` (true) to the project.
- **Dear ImGui is private to it.** It is the only module that includes `imgui.h` (no header of the editor does). The
  package is `imgui v1.92.9+b-docking`, with none of its backends: the editor does their work through the contracts.

| Folder | Holds |
|---|---|
| `editor/` | `EditorPlugin` (the two hooks, the frame, the default layout), `EditorState` (everything kept between frames) |
| `gui/` | `GuiContext` (the ImGui context and the UI fonts), `Theme` (the Catppuccin Mocha colors, shapes and tone widgets), `GuiInput` (the window's events to ImGui and to the game), `GameViewport`, `GuiMesh` (ImGui's draw data and textures for the renderer) |
| `play/` | `PlaySession`: Play, Pause, Step and Stop; `OnlinePlay`: the steps of Play online |
| `scene/` | `SceneEdit`: the capsule changes (copy a tree with a node removed or moved, add, rename), commit, open, save, new files; `Isolation`: a capsule shown alone (prefab mode) |
| `console/` | `ConsoleLog` (the last lines, from any thread), `OutputCapture` (the process's output into it) |
| `fonts/` | JetBrains Mono Nerd Font, Regular (the text) and Bold (`gui::pushEmphasisFont()`: dark text on a filled button or chip, where thin strokes look faded), with its OFL license. Embedded in the program by `utils.bin2c`, so the editor needs no file at run time; the Nerd Font icons (Font Awesome in the Play controls) come with it |
| `shell/` | `TextEditor`: a file opened in an external text editor |
| `panels/` | The panels, `FieldEditor` (the widgets of a reflected value, the drag-and-drop of assets) and `WorldView` |

`play/`, `scene/`, `console/` and `shell/` include no ImGui: the tests (`tests/plugins/EditorPlaySessionTests.cpp`,
`EditorSceneEditTests.cpp`, `EditorIsolationTests.cpp`, `EditorConsoleTests.cpp`, `EditorPluginInfoTests.cpp`, `EditorAssetKindTests.cpp`) drive them without a window.

## How it reaches the screen and the window

The editor knows no GLFW and no Vulkan. What it needs from the backends is in the contracts, as plain data any tool can
use:

| Need | Where |
|---|---|
| Draw triangles over the frame | `IRenderer::drawMesh(const Mesh&)`: indexed triangles in framebuffer pixels, a texture and a clip per command (`contract/renderer/drawing/Mesh.hpp`) |
| A second picture of the world | `IRenderer::setScenePreview(preview)` draws the world again from another view into a texture of its own; `getPreviewTexture()` gives it |
| Show the game inside a panel | `IRenderer::setSceneSize(size)` draws the scene into a texture of that size instead of the window; `getSceneTexture()` gives it, `getSceneSize()` is the window the game sees |
| Read what the frame drew | `IRenderer::getFrameStats()` (`contract/renderer/stats/FrameStats.hpp`) |
| Copy and paste | `IPlatform::setClipboardText()`, `getClipboardText()` (GLFW implements them), behind ImGui's clipboard |
| See the window's events first | the graphics plugin's `PlatformEventFilter` resource |
| Run a frame over the game | the graphics plugin's `ToolFrame` resource |

All of them are optional in the contracts: a renderer without them ignores the meshes, draws the scene into the window
and reports nothing. The Vulkan renderer implements them (`rendering/MeshPass.hpp`, `shaders/mesh.vert` and
`mesh.frag`, and the scene texture in `VulkanRenderer`, see `docs/Vulkan.md`).

### The two hooks of the graphics plugin

The editor adds no system. It sets two resources the graphics plugin inserted, so it needs no order against the plugins
that draw (the scheduler refuses two Render systems that touch the `Backend` without an order, and the graphics plugin
owns it):

- **`PlatformEventFilter`** (`plugins/graphics/PlatformEventFilter.hpp`): `rtype.graphics.events` calls it for each event
  before it becomes an ECS event. It may change the event, or return false to hide it from the game. The renderer is
  resized by the real `Resized` events whatever it decides.
- **`ToolFrame`** (`plugins/graphics/ToolFrame.hpp`): `rtype.graphics.endFrame` calls it after the overlay, just before
  the renderer ends the frame, with the platform and the renderer. It is not called while the window is minimized.

The editor reads and writes the world from that callback, which is not a system with declared access: it runs on the
main thread where nothing else runs, and the scheduler is serial. A parallel scheduler would need it to be an exclusive
system.

## Editing, playing

The editor starts **editing**: `Schedules::setPaused(true)` (`ecs`). A paused frame runs Startup, PreUpdate, PostUpdate
and Render, but neither FixedUpdate nor Update: the events come in, the global transforms, the animations and the
particles still run, the gameplay does not move. The clock still measures the frames, so `Time::delta` keeps its value
(an effect previews at its speed) and the fixed ticks of a paused frame are dropped, never caught up.

| Button | `PlaySession` | What happens |
|---|---|---|
| Play | `play()` | Remembers the entities alive, runs the simulation |
| Pause / Resume | `togglePause()` | While playing |
| Step | `step()` | While playing and paused: the next frame runs whole, with one fixed tick (`Schedules::step()`) |
| Stop | `stop()` | Pauses, despawns every entity spawned since Play, and respawns the open scene from its capsule (shows it again if its root is gone) |

Stop does not put back what is not the scene: the entities that were there before Play keep what the game did to them,
and the resources and the scripts' state stay as they are.

### Playing online

The toolbar has a choice next to Play: **Standalone** (the default, above) or **Online**, locked while a Play is under
way. Online tests the real thing, a server and its clients:

1. Play remembers the open scene, then starts the child server with the options of the Server panel (game folder, port,
   flags), unless one already runs; the bar shows "Starting server..." and a Cancel button.
2. Once the server answers on its admin channel (`ServerStatus::kRunning`), `OnlinePlay` asks the Engine for
   `requestReload(RunMode::kClient, <the program's flags> --host 127.0.0.1 --port <port>)`. The App is rebuilt between
   two frames, as for any reload: the editor is built again as a client, joins the server and runs at once.
3. Stop rebuilds the editor standalone and opens the scene again. **The server keeps running** (and its clients): stop
   it in the Server panel. The next online Play joins it without starting another.

While online the editor is a client, so nothing writes the replicated data (rule 6): no scene is edited or opened, the
Hierarchy lists the replicated entities, the Inspector shows them read-only, and the server's world is edited from the
Server panel's World tab. Pause and Step act on the client's schedule only; the server goes on (pause it in the Server
panel). The game's `project.luau` has to read `engine.args.host` and `engine.args.port` into `network.host` and
`network.port`, as it does for `aylo-runtime`: the editor passes the connection that way.

The state that must outlive the rebuild lives in an `EditorSession`, created by the program (`aylo-editor`'s `main()`)
and given to every `EditorPlugin` through `EditorOptions::session`; without one (tests), each plugin makes its own. If
the client cannot be built, the Engine goes back to standalone (`Engine::requestReload(mode, args)`) and `OnlinePlay`
gives up with a message in the bar.

### The Server panel

The editor supervises the server as a child process (`process::ChildProcess`, which pipes its output and always ends
it: the editor's exit stops every child). `ServerSupervisor` runs
`aylo-runtime --server <game> --port N --admin-port M --admin-token T`, with a free admin port and a random token, and
`AdminSession` talks to it over the admin channel (`docs/network/design.md`), retrying the datagrams the network drops.
Debug runs only, like the channel. The Server panel (View menu) has Start, Stop and Restart, the status (tick,
entities, uptime) and these tabs:

- **Log**: the child's output, also in the Console.
- **Players**: the connected clients (id, address, RTT, dropped messages) and a Kick button.
- **Network**: sliders for the added latency, jitter and loss, on both directions of the server's link.
- **World**: pause, step and resume the server; list, inspect and edit the entities (a field is sent as text, and the
  server decides: it stays authoritative), spawn and despawn.
- **Clients**: `ClientLauncher` starts N detached `aylo-runtime <game> --host 127.0.0.1 --port N [flags]` windows, each
  with its own flags (e.g. `--name p2`), lists them, kills one or all, and restarts them all when the server restarts.
  Their output reaches the Console as `[client N] ...`.

## Scenes

A scene is a capsule (`.scene`, `.actor`, `.capsule` under `<game>/assets/`, `docs/CapsuleFormat.md`): what the
editor edits is the capsule, the data the file holds, and the world only shows it.

- **Open.** `panels::openScene()` (a double-click in the Assets panel, or `editor.scene` at start) reads the file if the
  capsules do not know it yet (`CapsuleFiles::reload()`), then shows it as the level (`loadCapsule()`).
- **Change.** The Hierarchy and the Inspector change the capsule in the world's `Capsules`: a value through
  `Capsules::edit()`, the tree through `scene::copyCapsule()` (a copy with a node removed, or moved last under another,
  renamed if its new siblings have its name) then `Capsules::define()`. Either way the capsule is named in
  `ReloadedCapsules`, and the capsule plugin's Render system respawns what was spawned from it in the next frame,
  nested instances through their outermost capsule. That system and its queue are there with or without hot reload.
- **Save.** `scene::save()` writes `format::writeCapsule()` to the file (comments are lost, see the format). With hot
  reload on, the engine reads the saved file again, which changes nothing.
- **Names.** A capsule's spawn gives each entity a `rtype.Name` (`ecs/entity/Name.hpp`), the name of its node, the
  file's name for the root, unless the node gives one. In a scene the name *is* the node's: renaming changes the node.

## Prefab mode

A double-click on an `.actor` (or a `.capsule`) in the Assets opens it **alone**, in a window of its own, the Prefab
window, while the Game panel goes on showing the game (`panels::openPrefab()`, `scene::isolate()`). The capsule is
spawned under an anchor entity at `kIsolatedOrigin` (100000, 100000), far from anything a game places and near enough
for a float to place it to a hundredth of a unit. The anchor holds the offset, so the capsule's own values are those of
its file, and a respawn after an edit keeps it isolated (a reload keeps the root's other components, its `Parent`
included).

The window shows the renderer's **preview** (`IRenderer::setScenePreview()`): every frame, the world is drawn a second
time, from a view centred on the capsule's root (`scene::centerOf()`, its GlobalTransform) at `EditorState::prefabZoom`
pixels per world unit (the wheel changes it), into a texture of the window's size. The game's camera is untouched. The
Hierarchy and the Inspector edit the capsule (`EditorState::getEdited()`), Save writes it, and its instances in the scene
respawn with it. **Back**, the window's cross, opening a scene or Play end it (`scene::endIsolation()`). A `.scene` opens
as the level.

The preview draws the world, so what the game places near the capsule's place would show too: nothing does that far
away. A parallax layer follows the game's camera, not the preview, so it stays out of it.

## Console

`console::OutputCapture` points the process's standard output and error (file descriptors 1 and 2) at a pipe a thread
reads: the thread writes everything to the terminal as before, and the lines to a `console::ConsoleLog`, which the
Console panel shows, its errors and warnings in colour (a line that says "error" or "warning"). It catches everything:
the C library's output (Luau's `print`), C++ streams, the libraries' messages (the Vulkan loader). Standard output is
made line buffered while captured. It is the process's own output, so there is one capture, owned by the editor, and the
descriptors are put back when it goes. A process the editor starts (the text editor) is started with its output thrown
away, so it never holds the pipe. `EditorOptions::captureOutput` turns it off (the tests do).

## Opening a file in a text editor

"Open in text editor" (the Assets' menu, the Hierarchy's header) runs the `editor.textEditor` command on the file, in the
background (`shell::buildOpenCommand()`): Visual Studio Code by default, through `open -a "Visual Studio Code"` on macOS
(it needs no `code` command installed) and `code` elsewhere.

## The panels

| Panel | File | What it does |
|---|---|---|
| Toolbar | `panels/ToolbarPanel.cpp` | One padded row (`gui::beginToolbar()`): on the left File (Save scene, Ctrl+S; Quit, through `IPlatform::close()`) and View (panels, Reset layout), as flat buttons opening popups; in the center the mode chip and the last message; on the right the Standalone / Online choice and Play / Pause / Step / Stop. The three groups are placed by `gui::beginToolbarGroup()` |
| Prefab | `panels/PrefabPanel.cpp` | The capsule opened alone, from the renderer's preview, centred; the wheel zooms; its cross goes back to the scene |
| Game | `panels/GamePanel.cpp` | Asks for a scene texture as big as its content, draws it as an image, tells the input bridge where it is (`GameViewport`) |
| Hierarchy | `panels/HierarchyPanel.cpp` | Editing: the scene's node tree; drag a node onto another to move it, drop a capsule file to place an instance, right-click to add or delete. Playing: the world's entities by their `Parent` and `Children`, named by their `Name`, or a flat list filtered by component name |
| Inspector | `panels/InspectorPanel.cpp` | Editing: the selected node's name, side, instance and components (added with their type's default value, removed, edited). Playing: the selected entity's components, the same way, lost at Stop |
| Assets | `panels/AssetsPanel.cpp` | A file explorer: on the left a small tree of the folders of `<game>/assets`; on the right the folder shown as a grid of icons (one per file type, told by `kindOf()` in `panels/AssetKind.cpp`, drawn with Nerd Font glyphs), with a breadcrumb, an up button and an icon size slider. Click selects, double-click enters a folder or opens a capsule, drag carries a file's path, right-click opens the menu (new scene / actor / folder, open, rename, delete) |
| Console | `panels/ConsolePanel.cpp` | The program's output: filters by kind (info, warnings, errors) and by text, Clear, right-click to copy a line, follows new lines unless scrolled up |
| Particles | `panels/ParticlesPanel.cpp` | The effects of `ParticleEffectAssets`: edit the spawning, each particle and the forces, preview at the middle of the view, Copy as Luau |
| Stats | `panels/StatsPanel.cpp` | `io.Framerate`, the plot of the last frame times, `getFrameStats()` |
| Plugins | `panels/PluginsPanel.cpp` | The plugins added to the App (`App::getPlugins()`), in the order recorded: kind (engine for an `rtype.` name, else game), requires, required by, a name filter, and the registered plugins not added. The data is gathered by `collectPlugins()` (`panels/PluginInfo.cpp`), without ImGui |

`FieldEditor::editFields()` shows a reflected value field by field (`TypeInfo::fields`). It works on a byte copy of the
value, which shares what the value owns (a list's elements): only plain fields are changed in it, and the caller writes
it back whole (`Components::getMut()`) or copies it into the capsule (`ComponentValues::set()`, with the type's hooks).
A list is shown and not edited. An asset field shows the path the capsules know it by (or the effect's name), takes a
file dropped from the Assets panel, and loads a dropped image as a texture (`TextureAssets::load()`) so it shows at once.
Parent, Children and CapsuleInstance are never shown, added or removed; in a scene, neither are Name (the node's name)
and GlobalTransform (computed).

### Particles

Effects are made by scripts (`engine.assets.particle`), so they have no file the editor could save. The Particles panel
changes the effect in memory (`ParticleEffectAssets::replace()`, which bumps the effect's version: the emitters playing
it restart with the new one, `ParticleEmitter` and `EmitterState::effectVersion`), and **Copy as Luau** puts the
`engine.assets.particle` table of what it edits on the clipboard, to paste in the script. The curves, the texture and
the blending are kept and not edited. Preview spawns a `ParticleEmitter` at the middle of the `VisibleArea`; as the
particles run in PostUpdate and Render, they play while editing.

## One frame

`ToolFrame` runs `runFrame()` in `EditorPlugin.cpp`:

1. Open `editor.scene` on the first frame (the capsule files are read once every plugin is built).
2. Tell ImGui the size of the window, the density of its pixels, the last frame's duration and the clipboard, and
   scale the panels to the screen (`gui::interfaceScale()`): the platform's content scale (2 on a screen set to 200%)
   over the framebuffer to window ratio. Where the window is measured in points (macOS, Wayland) that ratio already
   covers it, so the scale is 1; where both are pixels (X11, including through XWayland, and Windows) it is the
   content scale. `GuiContext::setScale()` rebuilds the style from its defaults at that scale (`ScaleAllSizes()`) and
   sets `FontScaleDpi`, so ImGui renders its glyphs again at that size and the text stays sharp.
3. `ImGui::NewFrame()`, the toolbar, the dockspace over the window (the panels are docked as by default on the first
   frame if no layout was kept; the layout file is `<cache>/rtype/editor/layout-<version>.ini`, its version raised when
   the panels change), then the panels.
4. `ImGui::Render()`, `gui::updateTextures()` (ImGui's font atlas, sent whole when it grows) and `gui::buildMesh()`, one
   `Mesh` with 32-bit indices, handed to `drawMesh()`.

## The input bridge

`gui::GuiInput::filter()` is the `PlatformEventFilter`. Every event feeds ImGui first. Then:

- the mouse reaches the game while it is over the picture of the game, or a button pressed there is still down, with
  its position moved to the picture's origin and scaled to the scene's pixels;
- keys and text reach it while the Game panel has the focus and no ImGui text field wants the keyboard, and a key
  release always does, so no key stays down in the game;
- `Resized` becomes the size of the scene, the window the game sees (the graphics plugin also gives the game that size
  as `ViewInfo::framebuffer` and `window`, and bases the `VisibleArea` on it);
- everything else (the window closing, the focus, the gamepad) passes.

## Settings

| Key | Default | Meaning |
|---|---|---|
| `editor.scene` | none | The capsule file, in the assets, opened at start: a `.scene` as the level, anything else alone |
| `editor.textEditor` | Visual Studio Code | The command that opens a file, its arguments included: `"code"`, `"subl"`, `"open -a TextEdit"` |

The section is read when the editor is built (`readEditorOptions()`), and an unknown key in it is an error.

The window is titled `AYLO Editor - <Game>`, where `<Game>` is the game's `window.title`; with none, `AYLO Editor`.

## Limits

- One window: no ImGui viewports outside it. A renderer without the tool calls of `IRenderer` shows no panel.
- No undo yet. No gizmos in the Game panel: positions are edited in the Inspector.
- Entity fields are shown and not edited; neither are lists. An entity with no component is not listed.
- Particle effects are edited in memory only (Copy as Luau to keep them).
- The App is built again on `Engine::requestReload()`: the editor is too, with its ImGui context, and loses its
  selection and the open scene (Play online and Stop bring the scene back; a hot reload does not). The child server and
  the Console survive: they are in the `EditorSession`.
- Online, edits are limited to the Server panel's World tab, and Play online needs a debug run (the admin channel).
