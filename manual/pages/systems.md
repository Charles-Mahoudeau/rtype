# Systems

A system is a function that runs on a schedule, on the entities that match a query. You declare one with
`engine.ecs.system`.

```luau
engine.ecs.system("game.MoveShips", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.Ship", "rtype.Transform" }, with = { "net.PlayerInput" } },
	calls = { "rtype.replication.readInput" },
	after = { "net.applyInputs" },
	run = function(ctx)
		for entity, ship, transform in ctx.query do
			local move = ctx.call("rtype.replication.readInput", { entity = entity, action = "move" }).vector
			transform.position = { x = transform.position.x + move.x * 4, y = transform.position.y - move.y * 4 }
		end
	end,
})
```

## The declaration

| Option | Meaning |
|---|---|
| `phase` | **Required.** When it runs, see below. |
| `query` | **Required.** Which entities it visits; `{}` for a system that visits none. |
| `run` | **Required.** The function, called with `ctx`. |
| `side` | `"server"` (default), `"client"` or `"both"`: where it runs. |
| `resources` | `{ read = {...}, write = {...} }`: the resources it uses. |
| `events` | `{ read = {...}, send = {...} }`: the events it reads or sends. |
| `calls` | The [engine calls](engine-calls.md) it makes with `ctx.call`. |
| `after`, `before` | Names of systems it must run after or before. |

Any other option is an error naming the system. A system must declare everything it touches: using an undeclared
resource, event or call is an error.

## Phases

A frame runs its phases in this order.

| Phase | Use |
|---|---|
| `Startup` | Once, before the first frame: setup, initial entities. |
| `PreUpdate` | Polling the window and input, receiving network packets. |
| `FixedUpdate` | Gameplay at a fixed rate; also the physics and the server's network send. |
| `Update` | Once per frame: animation, camera, UI. |
| `PostUpdate` | Transform propagation, sending input (client). |
| `Render` | Extracting and drawing. |

Gameplay on the server goes in `FixedUpdate`; what a client shows goes in `Update`. Use `after` and `before` to order
systems of the same phase, including the engine's (the physics names are in [Physics](physics.md)).

## Queries

```luau
query = {
	read = { "rtype.Transform" },
	write = { "game.Ship" },
	optionalRead = {}, optionalWrite = {},
	with = { "net.Owner" }, without = { "game.Dead" },
	added = { "game.Laser" }, changed = { "game.Ship" },
}
```

| Term | Meaning |
|---|---|
| `read`, `write` | The entity must have the component, and the loop gets a view of it (read-only, or writable). |
| `optionalRead`, `optionalWrite` | A view, or `nil` when the entity does not have it. |
| `with`, `without` | The entity must have, or must not have, the component. No view. |
| `added` | The entity must have it, and it was added since this system last ran. |
| `changed` | The entity must have it, and it was added or written since this system last ran. |

`for entity, a, b in ctx.query do` gives the entity, then one view per `read`/`write` term in the order read, write,
optionalRead, optionalWrite (inside each, in the order you listed them). The views are valid only during the call to
`run`. An unknown term name is an error.

## `ctx`

| Field | What it gives |
|---|---|
| `ctx.query` | The loop over matching entities. |
| `ctx.time` | `delta` (seconds of the frame), `fixedDelta` (the fixed step), `elapsed`, `tick` and `alpha` (progress between two fixed steps). `ctx.delta` and `ctx.fixedDelta` also exist. |
| `ctx.commands` | Deferred changes to the world, see below. |
| `ctx.resources` | The resources the system declared: `ctx.resources["rtype.Input"]` is a view (read-only unless declared in `write`). |
| `ctx.events` | `ctx.events.read(name)` loops over the events not read yet; `ctx.events.send(name, fields)` sends one. Both need the name in `events`. |
| `ctx.call(name, args)` | Makes an engine call the system listed in `calls`. |

### Commands

Changes to the world are recorded and applied after the system, not during the loop (a spawned entity cannot be queried by the loop that spawned it):

```luau
local ship = ctx.commands:spawn({
	["game.Ship"] = { position = position, player = 0 },
	["rtype.Transform"] = { position = position },
})
ctx.commands:add(ship, "rtype.RigidBody", { type = 1 })
ctx.commands:setParent(child, parent)
ctx.commands:removeParent(child)
ctx.commands:despawn(ship)
```

`spawn` takes a table of component name to fields (fields left out keep their defaults) and returns the entity. It also
builds the engine's components, such as `rtype.Collider` or `rtype.TextRenderer`. A command that names an unknown
component, or that a client system may not write, is an error and records nothing.

### Events

An event is a message between systems. Declare the ones you use under `events`, then:

```luau
events = { read = { "rtype.CollisionStarted" }, send = { "rtype.PlayMusic" } },
run = function(ctx)
	for collision in ctx.events.read("rtype.CollisionStarted") do
		print(collision.first, collision.second)
	end
	ctx.events.send("rtype.PlayMusic", { sound = music, crossfade = 1 })
end
```

Events are local to one machine and are not replicated: to tell the other side something, put it in a replicated
component. Declaring your own event types from Luau is not available yet (*planned*); you use the engine's, such as
`rtype.CollisionStarted`, `rtype.PlayMusic`, and the network ones in [Networking](networking.md).

## Where a system runs

Scripts load on the server and on every client, so a system is declared on both. Its `side` decides where it runs. A
client system may not write replicated or server-only components. What exists on only one side (sprites and audio on a
client) must be named only there: a file that uses client-only things starts with

```luau
if not engine.isClient then
	return
end
```

after declaring any component that must exist on both. Only engine calls may be missing on the side a system does not
run on. See [Networking](networking.md).

## Errors

A bad declaration (an unknown option, a missing `phase`, an undeclared name) is an error naming the system, raised when
the script loads, and the script fails to load. A mistake inside `run` (an undeclared resource, a call that was not
listed, a stale view) is a Luau error naming the cause. Read the output of the program.
