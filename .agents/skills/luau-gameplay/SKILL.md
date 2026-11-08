---
name: luau-gameplay
description: Write or change a game in Luau for the AYLO Engine - project.luau, components, systems, engine calls, setup forms, assets - without touching C++. Use when asked to "make a game", "write gameplay", "add a system", "add a component in Luau", "change the game scripts", or when working in a game/ folder.
---

# Write a game in Luau

Read `examples/games/AGENTS.md` first: you write Luau and `project.luau` only, never C++. Learn the API from `manual/pages/`
(the game developer manual); where it does not cover something yet, `docs/ecs/features/19-luau-bridge.md`,
`docs/scripting/plan.md` and `docs/engine/design.md` hold it. If neither says an API exists, it does not: file an engine
issue (`plan-issues` skill) instead of working around it.

## Folder layout

```
my_game/game/
  project.luau      settings, run before anything else (required)
  scripts/          the entries named in project.luau (default main.luau) and the files they require
  assets/           everything loaded, named by its path under assets/ (clients require the folder)
```

Run it: `./aylo-runtime --server my_game/game` and `./aylo-runtime my_game/game` (`./r-type_server` and
`./r-type_client` are wrappers for them); every `--name value` flag is
`engine.args.name` in `project.luau`. Model it on `examples/games/rtype_duel/game/`: `scripts/shared/` (the rules and the replicated declarations, required by both sides), `scripts/server/` and
`scripts/client/` (an entry `main.luau` each, plus the gameplay and the presentation).

## project.luau

Plain Luau that sets `engine.settings`; `engine.isClient`, `engine.isServer` and `engine.isDebug` say where it runs.
It chooses everything about the engine: `network.port`/`network.host`, `plugins` (the names the plugins give
themselves, such as `{ "rtype.Physics", "rtype.PhysicsDebug" }`; a client lists `rtype.graphics`, `rtype.Sprite`... too)
and their sections (`physics.gravity`...), on clients `platform`, `renderer`, `window.title`/`size`, `audio`... Unknown
keys are errors: a typo fails at start. Never change engine code to change a setting.

## Entry points and require

`engine.settings.scripts.main/server/client = "shared/main"` (paths in `scripts/`, no extension) name the files that run:
main, then server on a server, then client on a client; standalone runs all three. With none set, `scripts/main.luau`
runs. A file nobody requires never runs. Share data only with `require("@scripts/shared/x")`: every file has its own
globals, the standard libraries and `engine` are read-only, and a returned table is frozen (keep state in a local behind
functions). Never write a global to pass data.

## Components

```luau
engine.ecs.component("game.Ship", {
	fields = { position = "vec2", player = "u32" },
	replication = "everyChange",         -- "none", "everyChange" or "spawnOnly"
})
engine.ecs.component("game.Timer", { fields = { remaining = "f32" }, presence = "serverOnly" })
```

Declare the same components on both sides (the manifest checks it). `presence` is `serverOnly`, `clientOnly` or both.
The server is authoritative: a client system never writes replicated or server-only components.

## Systems

```luau
engine.ecs.system("game.MoveShips", {
	phase = "FixedUpdate",                -- required: Startup, PreUpdate, FixedUpdate, Update, PostUpdate, Render
	side = "server",                      -- "server" (default), "client" or "both"
	query = { read = { "game.Ship" }, write = {}, with = {}, without = {} },   -- required, {} for none
	resources = {}, events = {}, calls = { "rtype.replication.readInput" },
	run = function(ctx)
		for entity, ship in ctx.query do ... end
	end,
})
```

`ctx` gives `query`, `commands` (spawn, add, remove, despawn: deferred), `time`, `resources`, `events` and `call`.
A system must declare every call, resource and event it uses. A file runs only where an entry requires it: put what needs client-only plugins under the client entry, with no
`engine.isClient` guard. Components and input actions both sides must declare alike go in a shared module both entries require.

## Setup forms and calls

Declarative setup at load time, the same code on both sides (a server gets ids without loading anything):
`engine.assets.texture/atlas/sound/font/tilemap(...)` return the asset's id; `engine.input.action(name, type, {bind})`
declares inputs the clients send. Anything else a plugin offers is an engine call: `engine.call("rtype.x.y", {args})`
at load, `ctx.call` in a declared system. The list of calls is in the manual (and in
`docs/ecs/features/19-luau-bridge.md`).

## Method

1. Write down the data (components, which side), the inputs, the systems and the settings before coding.
2. Put shared rules in `shared/` (required by both entries); server gameplay in `server/`, presentation in `client/`.
3. Run a server and a client and play it. A script error is printed with the system and does not stop the game: read
   the output, fix, run again.
4. If something is missing from the API, stop, file the issue, tell the user. Do not edit C++.
5. Update `manual/` when you found it wrong or incomplete (`write-manual` skill).
