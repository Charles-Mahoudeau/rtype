# Reading and writing capsule files

The engine reads your capsule files when the game starts and again when you save one, and it can write a capsule back
as a file. This page says when each happens and what it does to the file. What you can write in a file is on the
[Capsule file format](capsule-format.md) page.

## When the game starts

With `rtype.Capsule` in `engine.settings.plugins`, the engine reads every `.capsule`, `.scene` and `.actor` file under
`assets/`, on the server and on every client.

- **After your scripts.** Files are read once every plugin and every script is loaded, so a file can use the
  components your scripts declare.
- **In path order, an instanced file first.** Files are read sorted by their path from `assets/`. A file that instances
  another (`instance="actors/turret.actor"`) gets that file read first, wherever it sorts, so its values are there to
  start from.
- **Once.** Reading turns the text into a capsule kept in memory. Spawning copies that capsule; it never reads the text
  again. Spawning the same bullet a thousand times reads its file once.
- **A mistake stops the game.** The first file that cannot be read stops the start, with the file, the line, the column
  and what is wrong (see [Errors](capsule-format.md#errors)). Fix it and start again.
- **A missing instance is found at spawn.** A file may instance a capsule that does not exist; the file still reads,
  and spawning it fails with a message that names the missing capsule.

A capsule's name is its file's path from `assets/`, so `assets/actors/bydo.actor` is spawned as `"actors/bydo.actor"`.

## When you save a file

With [hot reload](hot-reload.md) on (`engine.settings.hotReload`, the default in a debug build), the engine watches
the capsule files of `assets/` while the game runs, on each side.

| You... | The engine... |
|---|---|
| save a file | reads it again, with every file that instances it (at any depth), then respawns what was spawned from them |
| add a file | reads it: you can spawn it at once |
| save a file with a mistake | keeps the version it had, and the log says where the mistake is |
| delete a file | changes nothing: its capsule stays as it was until the game is built again |

What *respawns* means for what is already in the world:

- **The root entity stays the same**, so every place your scripts keep it is still right.
- The root gets the components of the new version. **Components your scripts added to the root stay.**
- **The root's children are despawned and spawned again** from the new version, including children your scripts
  attached to it.
- A capsule spawned inside another (an instance) respawns through the capsule that contains it, so that capsule's
  overrides still apply.

Each reload writes one line to the error output:

```
[HotReload] capsules: applied in 0.4 ms (modified assets/actors/turret.actor): capsule actors/turret.actor, capsule actors/boss.actor
[HotReload] capsules: failed in 0.2 ms (modified assets/actors/turret.actor): actors/turret.actor:3:25: unknown component 'game.Healht' (the last version is kept)
```

A system can also respawn one capsule by hand with `engine.capsule.reload(ctx, root)` (see
[Capsules](capsules.md#spawning-switching-levels-reloading)).

## When the engine writes a file

The engine can write any capsule back as a file. A capsule editor saves through it, so that what it saves is always
what the engine reads. Scripts cannot save a capsule.

The written file reads back as the same capsule, but it is not the text you typed. This file:

```
# The boss, with a turret on each side.
[node path="."]
game.Health = { value = 50, max = 100 }   # max is the default
rtype.Transform = {
    position = (0.0, 2.50),
    scale = (1, 1),
}

[node path="LeftTurret" instance="./actors/turret.actor"]
rtype.Transform = { position = (-40, 0), rotation = 1.5 }

[node path="Shadow" side=client]
game.Sprite = { texture = "sprites/boss_shadow.png", layer = -1 }
```

is written back as:

```
[capsule format=1]

[node path="."]
game.Health = { value = 50 }
rtype.Transform = { position = (0, 2.5) }

[node path="LeftTurret" instance="actors/turret.actor"]
rtype.Transform = { position = (-40, 0) }

[node path="Shadow" side=client]
game.Sprite = { texture = "sprites/boss_shadow.png", layer = -1 }
```

| What | How it is written |
|---|---|
| The header | `[capsule format=1]` is always written first |
| Nodes | in the same order, each with its full `path`; `instance` and `side` when set (`side` left out when `server`) |
| Components | in the same order, one per line |
| Fields | **only those that differ** from the component's defaults, or, on an instance node, from the instanced capsule's values: above, `max = 100` and `scale = (1, 1)` are the defaults, and `rotation = 1.5` is already the turret's; `{}` when none differs |
| Numbers | in their shortest exact form: `2.50` becomes `2.5`, `0.0` becomes `0` |
| Assets and instances | by their path, tidied: `./actors/turret.actor` becomes `actors/turret.actor` |
| Comments and blank lines | **dropped** |

So a file the engine saves loses its comments. Keep the notes you care about somewhere else, or keep hand-written files
out of the editor.

## See also

- [Capsule file format](capsule-format.md): everything you can write in a file.
- [Capsules](capsules.md): spawning, switching levels and reloading from a system.
- [Hot reload](hot-reload.md): what else the engine reloads while the game runs.
