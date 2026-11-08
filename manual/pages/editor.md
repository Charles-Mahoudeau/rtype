# Editor

The editor opens your game in a window of panels. You build your scenes there: you see what is in them, add, move and
delete what they hold, change their components and save them to their files. Then you press **Play** and the game runs
inside the editor, and **Stop** puts the scene back as you left it. It works like the editors you may know: what you
change while editing is saved, what you change while playing is lost at Stop.

## Start it

The editor is its own program, `aylo-editor`. You make your game in it, and you ship it with `aylo-runtime`:

```sh
./aylo-editor --game path/to/game
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
| **Assets** | The files of your `assets/` folder: open a scene, make new ones, rename, delete, drag a file |
| **Particles** | Your particle effects, to tune and preview |
| **Console** | What your game writes: your scripts' `print`, the engine's errors and warnings |
| **Stats** | The frame rate, the last frame times, and what the frame drew |

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

Double-click an `.actor` (or a `.capsule`) in the Assets: it opens **alone**, in the **Prefab** window, in the middle,
with nothing of your game around it, while the Game panel goes on showing your game. Turn the wheel over it to zoom.
The Hierarchy shows its nodes, the Inspector its components, and you edit and save it as a scene. Every instance of it in
the scene follows your changes. **< Back**, at the top of the Hierarchy, or the window's cross, returns to the scene. The
Prefab window floats the first time: dock it where you like, the editor remembers. A `.scene` you double-click opens as
the level.

What your scripts add to an entity in `PostUpdate` shows there too: give the look of an actor (its sprite) in
`PostUpdate` rather than `Update`, as `Update` does not run while you edit.

### Open the file

**Open in text editor**, at the top of the Hierarchy or in the menu of a file in the Assets (right-click), opens the file
in Visual Studio Code. Name another editor in `project.luau`:

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
