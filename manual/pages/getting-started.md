# Getting started

## Run a game

The engine builds one generic program, `aylo-runtime`, and copies it to the repository root. It runs a game folder:
`game/` in the current directory, or the folder you give as first argument. It runs the client by default and the server
(no window, no graphics) with `--server`. At the repository root, the scripts `r-type_server` and `r-type_client`
(`.bat` files on Windows) start it for you on the matching side, so `./r-type_server` is `./aylo-runtime --server`.

```sh
xmake build -y
./aylo-runtime --server examples/games/rtype_duel/game
./aylo-runtime examples/games/rtype_duel/game      # one per player
```

The folder must contain a `project.luau`. A client also needs the `assets/` folder and refuses to start without it; a
server can run with `project.luau` and `scripts/` alone. A relative path is resolved against the working
directory when the program starts.

## The command line is yours

`aylo-runtime --version` prints the version of the engine, the commit it was built from and the library version, and
stops: quote it in a bug report. The runtime keeps `--server` (it selects the server side), `--ticks` (see below) and `--version`, and the
engine keeps the game folder, which must come first. Every other `--name value` (or `--name=value`, or a bare
`--name`) is given to `project.luau` as `engine.args`, so your game decides its own options, and `--server` and `--ticks` are not among them:

```luau
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end
```

```sh
./aylo-runtime --server my_game/game --port 4243
./aylo-runtime my_game/game --host 10.0.0.2 --port 4243
```

Values are typed: `true` and `false` are booleans, a decimal number is a number, anything else is a string. A flag given
twice keeps its last value. Any other argument is an error.

## What runs where

In `project.luau`, three flags tell you where the file is running: `engine.isClient`, `engine.isServer` and
`engine.isDebug`. They are also available to your scripts.

A process runs the scripts of its own side only: the server runs the `server` entry, a client the `client` entry, and
both run the shared `main` entry first (see [Your scripts](#your-scripts)). So a server file never needs a test of
`engine.isServer`: the client does not load it.

## Your first game

Create `hello/game/project.luau`:

```luau
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.platform = "glfw"
	engine.settings.renderer = "vulkan"
	engine.settings.window.title = "Hello"
end
```

and `hello/game/scripts/main.luau`:

```luau
engine.ecs.component("game.Counter", { fields = { value = "u32" } })

engine.ecs.system("game.Count", {
	phase = "FixedUpdate",
	query = { write = { "game.Counter" } },
	run = function(ctx)
		for entity, counter in ctx.query do
			counter.value += 1
		end
	end,
})
```

Run `./aylo-runtime --server hello/game`. Read on to learn what every part of it means.

## Your scripts

Nothing under `scripts/` runs by itself. The game starts from **entry points**, files you name in `project.luau`,
and everything else runs because an entry (or a file an entry requires) calls `require` on it. A file nobody requires
is never run.

```
my_game/game/
  project.luau
  scripts/
    shared/main.luau       components and input actions both sides declare
    server/main.luau       the server's gameplay
    client/main.luau       what the client shows and plays
```

```luau
-- project.luau
engine.settings.scripts.main = "shared/main"
engine.settings.scripts.server = "server/main"
engine.settings.scripts.client = "client/main"
```

A process requires `main`, then `server` if it is a server, then `client` if it is a client. A standalone process is
both, so it runs all three, and a module that two of them require runs once. A game with a single side, or a first
game like the one above, sets nothing: the entry is `scripts/main.luau`. See
[Project settings](project-settings.md#scripts) for the rules and [Modules and `require`](modules.md) for how files
share data.

When your game is ready, [Shipping your game](shipping.md) explains what to give your players.

## Stopping the game and running a test

To know that the player closed the window, read the event `rtype.Closed` in a system (declare it under `events = { read = { "rtype.Closed" } }`). The engine does not stop the game by itself: send `rtype.AppExit` in answer, after saving for example.

A script stops the process with the event `rtype.AppExit`. The game finishes the frame it was sent in, and `aylo-runtime`
returns `code` as its exit code (`0` is success; send another number to fail):

```luau
engine.ecs.system("game.Quit", {
	phase = "Update",
	query = {},
	events = { send = { "rtype.AppExit" } },
	run = function(ctx)
		ctx.events.send("rtype.AppExit", { code = 1 })
	end,
})
```

If several are sent, the first one read gives the code. `--ticks <n>` runs at most `n` frames and then exits with `0`,
unless an `rtype.AppExit` came first: with it a test game checks its rules in a system and sends `AppExit` with a
non-zero code when one fails, so a CI job just checks the exit status:

```sh
./aylo-runtime --server --ticks 600 my_game/tests
```

The limit counts frames, not `FixedUpdate` steps: a frame runs `FixedUpdate` zero, one or several times depending on how
long it took, so `--ticks 600` does not mean 600 fixed steps. To check something after a given amount of simulated time,
count it in your own system (for example from the time the fixed step gives you) and send `AppExit` then.

`--ticks` takes a whole number and may be given only once; a missing, negative, non-numeric or too large value, or a
second `--ticks`, stops the program with the exit code `84` before the game starts. With hot reload on, a rebuild that
fails during a `--ticks` run also ends it with `84`, instead of waiting for you to fix the files as a normal run does.
