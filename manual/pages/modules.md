# Modules and `require`

A script file can use another one with `require`. The other file is a **module**: it runs once, and what it returns is
what every `require` of it gives back. This is how you share configuration and helpers between files.

!!! note
    This is a transitional state. Every file under `scripts/` still runs automatically, in sorted path order, and
    global variables are still shared between files. Entry points chosen in `project.luau` and one set of globals per
    file come next. Write new code with `require` and `local` so it keeps working when they land.

## A first example

`scripts/shared/config.luau`:

```luau
return {
	shipSpeed = 4,
	name = "Ship",
}
```

`scripts/ship.luau`:

```luau
local config = require("./shared/config")

engine.ecs.component("game." .. config.name, { fields = { speed = "f32" }, defaults = { speed = config.shipSpeed } })
```

## Writing a path

A path is a string with no extension. It names `<path>.luau`, or `<path>/init.luau` when `<path>` is a folder.

| Form | Meaning |
|---|---|
| `"./x"` | `x` next to the file that asks |
| `"../x"` | `x` in the folder above (as many `../` as you need, never above `scripts/`) |
| `"@scripts/x"` | `x` from the root of `scripts/`, the same from every file |
| `"@self/x"` | `x` inside the folder of the file that asks (in an `init.luau`, inside its own folder) |

Rules:

- `/` is the only separator. A backslash, an extension (`"./x.luau"`), an empty segment or a `.` in the middle is an
  error.
- A folder with an `init.luau` is required by its name: `require("@scripts/ui")` loads `scripts/ui/init.luau`. Inside
  that `init.luau`, `./` means the siblings of the `ui` folder, and `@self/` means what is inside `ui`.
- If both `x.luau` and `x/init.luau` exist, `require("./x")` is an error: rename one.
- The case of every part must be the exact one on disk. `require("./Config")` for `config.luau` fails on every system,
  even where the file system would accept it, so a game written on Windows or macOS also works on Linux.
- Nothing outside `scripts/` can be required: not with `..`, not with a link that points out.

## What a module returns

A module returns nothing (a `require` gives `nil`) or one value. Returning two or more is an error.

A returned table is **frozen**: reading works, writing a field raises an error. The freeze is shallow, so a table inside
it can still be changed. Keep state that changes in a local and give functions to change it:

```luau
local score = 0

return {
	add = function(points)
		score += points
	end,
	get = function()
		return score
	end,
}
```

## Caching

A module runs the first time it is required, and only once. Every later `require` returns the same value, so
`require("./a")` and `require("@scripts/a")` share one instance when they name the same file.

What a module declares (components, systems, plugins) belongs to that module. Systems run on the schedule, not while a
file loads, so a system calls `require` only for modules already loaded: require what you need at the top of the file,
not inside `run`.

## Cycles

If `a` requires `b` and `b` requires `a`, the error shows the chain:

```
cycle: scripts/main.luau -> scripts/a.luau -> scripts/b.luau -> scripts/a.luau
```

Move what both need into a third module.

## Errors

An error reads `<file>:<line>: require("<path>"): <reason>`, so it names the file that asks. A failed module is not
cached: fix it and it loads again.

| Reason | Cause |
|---|---|
| `module not found: ...` | no file for the path; both candidates are listed |
| `ambiguous: both x.luau and x/init.luau exist` | two files for one path |
| `escapes scripts/` | the path leaves the scripts folder |
| `case mismatch: the file is 'Config.luau'` | wrong case; the message gives the real name |
| `unknown alias '@name' (known aliases: ...)` | the `@name` does not exist |
| `invalid path: ...` | backslash, extension, empty segment, or a misplaced `.` or `..` |
| `cycle: ...` | the modules require each other |
| `returned N values` | a module returned more than one value |
| `failed: <error>` | the module itself raised an error or has a syntax error |

When a file fails while the game starts, the game stops with `Script 'scripts/<file>' failed: ...`. Files are named
`scripts/<path>` in every message.

## Editor setup

Every game folder has a `.luaurc` next to `project.luau`:

```json
{
	"aliases": {
		"scripts": "./scripts"
	}
}
```

With it, [luau-lsp](https://github.com/JohnnyMorganz/luau-lsp) understands `@scripts/...` paths and completes them. The
template game has it already; copy it to your game folder if you started another way. The engine itself does not read
this file: `@scripts` always exists.

To add an alias of your own, declare it in `project.luau` with `engine.settings.scripts.aliases`
(see [Project settings](project-settings.md#scripts)) and mirror it here, with the target relative to the game folder:

```json
{
	"aliases": {
		"scripts": "./scripts",
		"shared": "./scripts/shared"
	}
}
```

## See also

[Hot reload](hot-reload.md) for what happens when you save a module (the files that require it run again too), and [Systems](systems.md).
