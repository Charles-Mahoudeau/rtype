# Getting started

## Run a game

The engine builds one generic program, `aylo-runtime`, and copies it to the repository root. It runs a game folder:
`game/` in the current directory, or the folder you give with `--game`. It runs the client by default and the server
(no window, no graphics) with `--server`. At the repository root, the scripts `r-type_server` and `r-type_client`
(`.bat` files on Windows) start it for you on the matching side, so `./r-type_server` is `./aylo-runtime --server`.

```sh
xmake build -y
./aylo-runtime --server --game examples/games/rtype_duel/game
./aylo-runtime --game examples/games/rtype_duel/game      # one per player
```

The folder must contain a `project.luau`. A client also needs the `assets/` folder and refuses to start without it; a
server can run with `project.luau` and `scripts/` alone. A relative `--game` path is resolved against the working
directory when the program starts.

## The command line is yours

`aylo-runtime --version` prints the version of the engine, the commit it was built from and the library version, and
stops: quote it in a bug report. The runtime keeps `--server` (it selects the server side) and `--version`, and the
engine keeps `--game`. Every other `--name value` (or `--name=value`, or a bare `--name`) is given to `project.luau` as
`engine.args`, so your game decides its own options, and `--server` is not one of them:

```luau
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end
```

```sh
./aylo-runtime --server --game my_game/game --port 4243
./aylo-runtime --game my_game/game --host 10.0.0.2 --port 4243
```

Values are typed: `true` and `false` are booleans, a decimal number is a number, anything else is a string. A flag given
twice keeps its last value. Any other argument is an error.

## What runs where

In `project.luau`, three flags tell you where the file is running: `engine.isClient`, `engine.isServer` and
`engine.isDebug`. They are also available to your scripts, which load on the server and on every client.

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

Run `./aylo-runtime --server --game hello/game`. Read on to learn what every part of it means.
