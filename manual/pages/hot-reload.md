# Hot reload

With `hotReload` on (the default in a debug build, see [Project settings](project-settings.md)), you edit the game
while it runs. The client and the server each watch the game folder and reload themselves, and each reload logs one line
on the error output: what reloaded, which files, how long it took, and what changed.

```
[HotReload] scripts: applied in 3.2 ms (modified scripts/ship.luau): system ship.move, component Ship
```

## What is followed

| Files | What happens |
|---|---|
| `scripts/**/*.luau` | The systems the file declares replace the running ones **in place**: entities and state are kept |
| `project.luau` | If it still runs, the game is **rebuilt** from the new settings; the world is lost |
| `assets/shaders/*` | A shader you loaded is compiled again; a typo keeps the last version on screen |
| `assets/**/*.capsule`, `.scene`, `.actor` | With `rtype.Capsule`, the file is read again with the files that use it, and what was spawned from them is respawned **in place**; a mistake keeps the last version (see [Reading and writing capsule files](capsule-files.md#when-you-save-a-file)) |

## Scripts

Saving a script runs it again. Its `engine.system` calls replace the systems of the same name, and a system you removed
from the file is removed from the game. If the file has an error, nothing changes and the error is logged.

When you save a file that other files [require](modules.md), those files run again too, directly or not, always after
the modules they use. The next `require` gives the new value, so the files that use a module see your change at once, and
you do not need to save them. Files that do not depend on it are left alone. Several files saved together reload as one.

If anything fails during that (a mistake in a file, a `require` of a file you deleted, a require cycle you just made),
nothing changes: the old code keeps running and the error is logged. Fix it and save again.

A module that no file requires any more is unloaded, and its systems are removed. A new file under `scripts/` runs
as soon as you save it while the game still loads every file of `scripts/`.

Some changes cannot be applied in place, so the game is rebuilt (and the log says why):

- a component whose fields changed, a new replicated component, or a component you removed;
- a system that changed phase, or that has the name of a system of another script;
- a deleted script file.

Top-level code of a script runs again on every save: setup forms are safe (assets are known by path), but a call that
spawns or changes the world at the top level will run again. Put such code in a system.

## When the game cannot be built

If a rebuild fails (a typo in `project.luau`, for instance), the game stops with the error and waits: fix the file and
save, and it builds again.
