# Networking

A networked game is one server and several clients, all running the same scripts. You do not write sockets or
messages: you decide **which side owns what**, and the engine replicates the rest.

## The model

- **The server is authoritative.** It runs the gameplay and owns the truth. A client never writes replicated data.
- **Clients draw and send inputs.** A client's only way to affect the game is its input actions; the server reads them
  and decides.
- **Replication copies data from the server to the clients.** A component declared with `replication = "everyChange"`
  or `"spawnOnly"` (see [Components](components.md)) appears on the clients, with its entity, and is updated.
- **Events do not cross the network.** To tell the other side something, put it in a replicated component (for example
  the duel's `game.Winner`, spawned by the server when a laser hits). Sending your own messages from scripts is
  *planned*.

## Sides

Every component says where it exists (`presence`), and every system where it runs (`side`):

| | `server` | `client` | `both` |
|---|---|---|---|
| system `side` | gameplay (default) | presentation | identical simulation on both |
| component `presence` | `serverOnly` | `clientOnly` | `both` (default) |

Both sides declare the same components, and a client that declares different ones is refused when it connects. The
rule of thumb:

- the server spawns the entities that matter and writes their gameplay components;
- a client system reacts to replicated entities (`added = {...}` finds the new ones) and adds what only it needs: a
  transform, a sprite, a sound, a text, using `clientOnly` components or the engine's own.

## Players and presence

The engine raises two events on the server, read like any event:

```luau
engine.ecs.system("game.SpawnPlayers", {
	phase = "Update",
	side = "server",
	query = {},
	events = { read = { "net.ClientConnected" } },
	run = function(ctx)
		for joined in ctx.events.read("net.ClientConnected") do
			ctx.commands:spawn({
				["game.Ship"] = { player = 0 },
				["net.Owner"] = { client = joined.client },
				["net.PlayerInput"] = {},
			})
		end
	end,
})
```

`net.ClientDisconnected` has the same `client` field. `net.Owner { client }` ties an entity to a client and is
replicated, so a client can find its own entities; `net.PlayerInput` receives that client's inputs on the server. A
client learns its own id with `rtype.replication.localClient`, which returns `{ connected, client }`.

## Inputs

1. Declare the actions with `engine.input.action` in a file both sides load. The actions are part of the network
   layout; the client and the server must declare the same ones.
2. On a client, the engine fills the input it sends from the actions of the same names. You write no input code.
3. On the server, read a player's action with `rtype.replication.readInput`, in a system that lists the call and
   queries entities with `net.PlayerInput`:

```luau
local move = ctx.call("rtype.replication.readInput", { entity = entity, action = "move" })
-- move.held, move.pressed, move.released, move.axis, move.vector
if move.vector.x > 0 then ... end
```

`pressed` is true on the tick the action went down. Order your system after `net.applyInputs`, which stores the tick's
inputs.

## Running

```sh
./aylo-runtime --server my_game/game --port 4242
./aylo-runtime my_game/game --host 127.0.0.1 --port 4242
```

Set `network.port` and `network.host` in `project.luau` from `engine.args` (see [Project settings](project-settings.md)).
Bytes from the network never crash the server: a malformed message is dropped.

## Not available yet

*Planned:* scripts sending their own network messages, a player name sent at connection, and prediction helpers.
