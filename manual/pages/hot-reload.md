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

## Scripts

Saving a script runs it again. Its `engine.system` calls replace the systems of the same name, and a system you removed
from the file is removed from the game. If the file has an error, nothing changes and the error is logged.

Some changes cannot be applied in place, so the game is rebuilt (and the log says why):

- a component whose fields changed, a new replicated component, or a component you removed;
- a system that changed phase, or that has the name of a system of another script;
- a deleted script file.

Top-level code of a script runs again on every save: setup forms are safe (assets are known by path), but a call that
spawns or changes the world at the top level will run again. Put such code in a system.

## When the game cannot be built

If a rebuild fails (a typo in `project.luau`, for instance), the game stops with the error and waits: fix the file and
save, and it builds again.
