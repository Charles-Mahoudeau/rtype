# Networking

A networked game is one server and several clients, all running the same scripts. You do not write sockets or
messages: you decide **which side owns what**, and the engine replicates the rest.

## The model

- **The server is authoritative.** It runs the gameplay and owns the truth. A client never writes replicated data.
- **Clients draw and send inputs.** A client's only way to affect the game is its input actions; the server reads them
  and decides.
- **Replication copies data from the server to the clients.** A component declared with `replication = "everyChange"`
  or `"spawnOnly"` (see [Components](components.md)) appears on the clients, with its entity, and is updated.
- **Events do not cross the network.** To tell the other side something lasting, put it in a replicated component (for
  example the duel's `game.Winner`, spawned by the server when a laser hits). For a moment or a request (a kick, a
  goal, a player picking a team), send [your own messages](#your-own-messages).

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

## Your own messages

A packet is a message your scripts declare, send and read. Use one for what is not state: a client asking for
something (join a team, buy an item), or the server telling the clients that something happened (a kick, a goal) so
they can play a sound or spawn particles.

### Declaring a packet

Declare every packet in a file both sides load, while the scripts load (not inside a system). Both sides must declare
it alike: a message whose declaration differs on the other side is dropped.

```luau
engine.net.packet("game.Join", {
	direction = "toServer",
	channel = "reliable",
	fields = { { "team", "u8" }, { "name", "string", 16 } },
})
engine.net.packet("game.Goal", {
	direction = "toClients",
	channel = "reliable",
	fields = {
		{ name = "team", type = "u8" },
		{ name = "scorer", type = "string", maxLength = 16 },
		{ name = "ownGoal", type = "bool" },
	},
})
```

| Key | Value | Default |
|---|---|---|
| name (first argument) | identifiers separated by dots, at most 64 bytes, such as `game.Join` | required |
| `direction` | `"toServer"` (a client sends it) or `"toClients"` (the server sends it) | required |
| `channel` | `"reliable"` (arrives, in order) or `"unreliable"` (may be lost; for frequent, short-lived news) | `"reliable"` |
| `fields` | a list of `{ name, type, maxLength? }`, positional or by key, in the order they travel | required |

A field's name is an identifier (letters, digits and `_`, not starting with a digit), unique in the packet, and not
`sender`. Its type is one of:

| Type | Holds |
|---|---|
| `bool` | `true` or `false` |
| `u8`, `u16`, `u32` | a whole number from 0 to 255, 65 535, 4 294 967 295 |
| `i32` | a whole number from -2 147 483 648 to 2 147 483 647 |
| `f32` | a finite number |
| `string` | text of at most `maxLength` bytes (default 64, at most 1000) |

A packet has at most 32 fields, and its largest message must fit in 1000 bytes: shorten its strings if the declaration
says it does not.

### Sending

`engine.net.sendToServer(ctx, name, values)` sends a `toServer` packet from a client system that lists
`"rtype.net.sendToServer"` under `calls`. `engine.net.sendToClients(ctx, name, values, client?)` sends a `toClients`
packet from a server system that lists `"rtype.net.sendToClients"`, to every client or only to `client` (an id from
`net.ClientConnected` or a packet's `sender`). `values` is a table with every field of the packet.

```luau
engine.ecs.system("game.PickTeam", {
	phase = "Update",
	side = "client",
	query = {},
	events = { read = { "rtype.UiClicked" } },
	calls = { "rtype.net.sendToServer" },
	run = function(ctx)
		for click in ctx.events.read("rtype.UiClicked") do
			if click.action == "menu.red" then
				engine.net.sendToServer(ctx, "game.Join", { team = 1, name = "Alice" })
			end
		end
	end,
})
```

Both return `true` when the message left, and `false` when nobody could get it: a client not connected yet, or a
server with no client ready (or not that `client`), or whose reliable queue is full. An **unreliable** packet from a client is also refused by the
server until the client's first snapshot arrived; send it again until it returns `true`. Reliable packets need no
wait.

### Receiving

`engine.net.received(ctx)` returns the packets that arrived this frame, as a list of `{ name, sender, values }`:
`values` is a table of the fields, and `sender` is the client that sent it (on the server; `0` on a client). The
system lists `events = { read = { "net.PacketReceived" } }`. A packet arrives on the side its direction names, in
`PreUpdate`, so any later phase sees it.

```luau
engine.ecs.system("game.JoinTeams", {
	phase = "Update",
	side = "server",
	query = {},
	events = { read = { "net.PacketReceived" } },
	run = function(ctx)
		for _, packet in engine.net.received(ctx) do
			if packet.name == "game.Join" then
				print(`client {packet.sender} joins team {packet.values.team} as {packet.values.name}`)
			end
		end
	end,
})
```

Each system reads the packets for itself: two systems calling `engine.net.received` both get every packet.

!!! warning "The server decides"
    The engine checks a packet's **shape** (the fields, their types and ranges) and drops a malformed one before any
    script sees it. Whether the request is allowed (the team is not full, the player has the gold) is your rule: check
    it on the server, using `sender`, and clean what a client wrote (a name) before you store it.

Sending a packet that is not declared, the wrong way, or with a missing or wrong field is a script error naming the
packet and the field (`'game.Join.team' must be a number`, `Packet 'game.Goal' goes to the clients: this side cannot
send it`); nothing is sent.

## Running

```sh
./aylo-runtime --server my_game/game --port 4242
./aylo-runtime my_game/game --host 127.0.0.1 --port 4242
```

Set `network.port` and `network.host` in `project.luau` from `engine.args` (see [Project settings](project-settings.md)).
Bytes from the network never crash the server: a malformed message is dropped.

## Not available yet

*Planned:* a player name sent at connection, and prediction helpers. Until then, send a name in a packet of your
own (the Haxball example's `game.Join`).
