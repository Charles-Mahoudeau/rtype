# Editor

The editor opens your game in a window of panels. You build your scenes there: you see what is in them, add, move and
delete what they hold, change their components and save them to their files. Then you press **Play** and the game runs
inside the editor, and **Stop** puts the scene back as you left it. It works like the editors you may know: what you
change while editing is saved, what you change while playing is lost at Stop.

## Start it

The editor is its own program, `aylo-editor`. You make your game in it, and you ship it with `aylo-runtime`:

```sh
./aylo-editor path/to/game
```

Nothing changes in your project. The editor runs both sides of the game in one process, the server's gameplay included,
with no network, and it is always a debug run: `engine.isDebug` is `true`, even in a release build. Your scenes are
[capsules](capsules.md), so list `rtype.Capsule`; the editor adds it if you forget.

To open a scene at once, name it in `project.luau`:

```luau
engine.settings.editor.scene = "scenes/level.scene"
```

An `.actor` named there opens alone (see below).

`engine.args.editor` is `true` when the game runs in the editor, if your project wants to know.

## The window

| Panel | What it is for |
|---|---|
| **Game** | The game, as big as the panel. While editing, it shows the scene; while playing, the game you play |
| **Hierarchy** | While editing, the tree of the scene's nodes. While playing, the entities of the game, by their parents |
| **Inspector** | The components of what the Hierarchy selected, to change, add and remove |
| **Assets** | An explorer for your `assets/` folder: the folders in a small tree on the left, the current folder as big icons on the right (one icon and color per kind of file). Double-click a folder to enter it, right-click to make a scene, an actor or a folder, rename, delete, or drag a file |
| **Particles** | Your particle effects, to tune and preview |
| **Console** | What your game writes: your scripts' `print`, the engine's errors and warnings |
| **Stats** | The frame rate, the last frame times, and what the frame drew |
| **Plugins** | The plugins your game loaded, in build order, each with its kind (engine or game), the plugins it needs and those that need it; a filter; and the plugins available but not loaded |

Drag a panel by its tab to dock it elsewhere. The editor remembers where they are, and **View**, **Reset layout** docks
them as they were the first time. **File**, **Save scene** (Ctrl+S) saves the scene; **File**, **Quit** closes the editor.

## Edit a scene

Double-click a `.scene`, `.actor` or `.capsule` file in the Assets panel to open it. Its nodes show in the Hierarchy and
in the Game panel. The gameplay waits: your systems in `FixedUpdate` and `Update` do not run while you edit, but your
`Startup` systems ran, and what follows the gameplay (positions of children, animations, particles) keeps running, so the
scene looks as it will.

| To | Do |
|---|---|
| Select a node | Click it in the Hierarchy |
| Add a node | Right-click a node, **Add child** |
| Delete a node | Right-click it, **Delete** (with what is below it) |
| Move a node | Drag it onto its new parent |
| Place a prefab | Drag an `.actor` (or any capsule file) from the Assets onto a node: it becomes an instance of it |
| Rename a node | Type its name in the Inspector, then Enter |
| Spawn it on one side | Pick **Side** in the Inspector: `server` (the default), `client` or `both` |
| Change a component | Drag a number, tick a box, type a text |
| Give an asset | Drag a file from the Assets onto the field: an image is loaded as a texture at once |
| Add a component | **Add component**, then pick it (search by name) |
| Remove a component | The cross on its header |

Every change shows in the Game panel at once. The Hierarchy says *not saved* until you save: **File**, **Save scene**, or
Ctrl+S. The file is written by the engine, as described in
[Reading and writing capsule files](capsule-files.md#when-the-engine-writes-a-file): only what differs from the
defaults is written, and the comments of a file you wrote by hand are lost.

Each entity a capsule spawns is named after its node (the file's name for the root), in the component `rtype.Name`.
The editor shows those names; your scripts can read them, and give names to the entities they spawn.

### Edit an actor alone

Double-click an `.actor` (or a `.capsule`) in the Assets: it opens **alone**, in an **Actor** tab, next to the Game tab,
with nothing of your game around it, while the Game tab goes on showing your game. Open several actors and each gets a
tab of its own (opening one again brings its tab back). Turn the wheel over it to zoom.
The Hierarchy shows its nodes, the Inspector its components, and you edit and save it as a scene. Every instance of it in
the scene follows your changes. **< Back**, at the top of the Hierarchy, or the window's cross, closes the
actor's tab and returns to the scene. Drag a tab where you like, the editor remembers. A `.scene` you double-click opens as
the level.

What your scripts add to an entity in `PostUpdate` shows there too: give the look of an actor (its sprite) in
`PostUpdate` rather than `Update`, as `Update` does not run while you edit.

### Open the file

**Open in text editor**, at the top of the Hierarchy or in the menu of a file in the Assets (right-click), opens the file
in Visual Studio Code (or VSCodium, or your system's default program, when `code` is not installed). If the command
cannot be found, the status line says so. Name another editor in `project.luau`:

```luau
engine.settings.editor.textEditor = "subl"  -- any command: it gets the file's path
```

### Fields

| Field | In the Inspector |
|---|---|
| Numbers, `bool`, `vec2`, `vec3`, `vec4` | Edit them: drag a number, or double-click it to type |
| Fixed arrays | One line per value, `name[0]`, `name[1]`... |
| Text of 255 bytes at most (`string`) | Edit it |
| Assets | The path of the file, or the name of a particle effect; drop a file from the Assets to change it |
| Entity references, lists | Read only |

## Play

| Button | What it does |
|---|---|
| **Play** | Runs the game from the scene as it is now, saved or not |
| **Pause** / **Resume** | Freezes the gameplay, or lets it go on |
| **Step** | While paused: one frame, with one fixed tick |
| **Stop** | Back to editing: everything spawned since Play goes, and the scene is spawned again as you left it |

While playing, the Hierarchy shows every entity of the game, and the Inspector changes them as they run. Those changes
are lost at Stop: they are for trying things. Click in the Game panel before you press the game's keys, or the keys go to
the editor.

| Input | The game gets it when |
|---|---|
| Mouse move, click, wheel | The mouse is over the Game panel, or a button pressed there is still down. Its position is in the game's pixels, from the panel's top-left corner |
| Keys and typed text | The Game panel has the focus (click it) and no field of the editor is being typed in |
| Window size | The game sees a window as big as the panel: that is the size it draws for, and the one its `Resized` events give |

## Console

The Console shows what your game writes: your scripts' `print`, the engine's messages, the renderer's. Errors are in
red and warnings in yellow. Tick **Info**, **Warnings** and **Errors** to choose what shows, type in **Filter** to keep
the lines that hold a text, **Clear** to start again, and right-click a line to copy it. Everything still goes to the
terminal too.

### Running Luau in the game

The line at the bottom of the Console runs Luau in your game, while it runs: type it and press Enter. The line is
written back with `>`, then what it gave. It is the quickest way to try a value.

A line that is an expression gives its values, written as Luau; any other line is run as a statement and gives nothing:

```luau
config.player.size
config.player.size.x = 10
```

```
> config.player.size
{ x = 1, y = 2 }
> config.player.size.x = 10
```

The line sees:

| Name | What it is |
|---|---|
| Each module your scripts loaded | By its file name: `config` for `scripts/shared/config.luau`. If two files share a name, the second is named with its whole path: `other_config` for `other/config.luau` |
| `modules` | Every loaded module, by its path without the extension: `modules["shared/config"]` |
| `engine` | `engine.call`, `engine.hasCall`, `engine.isClient`... but not `engine.ecs`: a line cannot declare components or systems |
| The standard libraries | `math`, `string`, `table`... |
| `self` | The entity selected in the Hierarchy, or `nil` when none is selected (or it is gone) |
| `entity(index, generation)` | Another entity, from the two numbers the Inspector shows. `entity(bits)` takes the two joined in one number |

A module returns its table frozen, so your scripts cannot write its top level. The Console can: it unfreezes the module
tables for the time of the line, then freezes them again, so the game still cannot write them. The game sees a new value
the next time it reads it. A script that copied the value when it loaded (a component default, an entity already
spawned) keeps its copy: change the table the script reads every frame, or reload with the new value.

Variables a line creates are gone with it: keep the state in a module table. An error shows in red with its message,
and the game goes on.

### Components of an entity

`self` and `entity(...)` give an entity whose components you reach with a dot, and whose fields you read and write like
the ones of a table. The change lands in the entity at once:

```luau
self.Ship.speed
self.Ship.speed = 3
self["game.Ship"].position
```

```
> self.Ship.speed = 3
> self.Ship.speed
3
```

A component is named by its whole name (`self["game.Ship"]`), or by its last part (`self.Ship`) when only one component
has it. If two components share the last part, the one the entity has settles it; if it has both, or none, the line
fails and lists the whole names to write. A component the entity does not have gives `nil`; a name no component has is an
error, and so is a field the component does not have, or a value of the wrong type.

A `vec2` or `vec4` field gives a handle, so you can write one part: `self.Transform.position.x = 0`. To move it whole,
write a table: `self.Transform.position = { x = 0, y = 0 }`. A `vec3` is a Luau `vector`, which cannot be changed
in place: write the whole value.

The handles live for the line that made them: keep a number, not `self.Ship`, for the next line. The Console only
reads while you play online: the editor is then a client, and a client never writes replicated data.

**Completion.** While you type a name, the names it can become show above the line as buttons: click one, or press
**Tab** to complete as far as all of them agree. It completes module names, the fields of a table
(`config.player.` offers `size`), the components of `self` and their fields (`self.Ship.` offers `speed`), `engine` and
the standard libraries. **Up** and **Down** browse the lines you ran before.

!!! note
    The line runs in the game the editor shows, on whichever side the editor runs. It is a debug tool: what it changes is
    lost when the game is rebuilt (a change to `project.luau`, Play online). While you play online the editor is
    a client of the server, so the line changes the client only, never the server.

## Particles

The Particles panel lists the effects your scripts make with [`engine.assets.particle`](particles.md). Pick one to tune
its spawning (rate, duration, loop, bursts, shape), each particle (lifetime, speed, angle, rotation, angular velocity,
size, color) and its forces (gravity, drag). The emitters playing it restart with your values at once, while you edit
too, and **Preview** plays it in the middle of the Game panel.

Effects live in your scripts, so the editor cannot save them. **Copy as Luau** puts the table of what the panel edits on
your clipboard: paste its fields in the `engine.assets.particle` call of your script. The curves, the texture and the
blending are kept as your script gives them.

## Pitfalls

!!! warning
    Stop only puts the scene back. What existed before Play and is not part of the scene keeps what the game did to it,
    and the resources and the state your scripts keep in variables stay as they are. If your game needs a clean start,
    have its `Startup` set it up, and restart the editor.

- An entity with no component does not appear in the Hierarchy.
- There is no undo yet: save often, and keep your scenes in version control.
- A scene cannot be opened while playing: press Stop first.
- The editor draws through the renderer you named. It needs one that can draw into a texture, which `vulkan` does.
- Leave the editor out of what you ship: `aylo-runtime` has no editor, and `aylo-editor` is not part of what you ship.

## See also

- [Capsules](capsules.md): what a scene is, and how a game spawns and switches them.
- [Capsule file format](capsule-format.md): what the editor writes.
- [Particles](particles.md): the effects the Particles panel edits.
- [Project settings](project-settings.md#editor): the `editor` section.
