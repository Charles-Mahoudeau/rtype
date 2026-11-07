# Particles

The particle plugin draws effects made of many small sprites: sparks, smoke, explosions, engine trails. You describe an
effect once, as a table, then play it from an emitter on an entity, or once at a point. Add the plugin in
`project.luau`:

```luau
engine.settings.plugins = { "rtype.SpriteDraw", "rtype.Particle" }
```

`rtype.Particle` lives on the **client** only: particles are something you see, not gameplay. The server has no
particles. It still runs `engine.assets.particle`, which checks the table and gives you the effect's id, so the same
script loads everywhere. Systems that play effects are client systems (`side = "client"`).

## A first effect

```luau
-- scripts/assets.luau: on every side, so the server knows the ids too
Game.effects = {
	sparks = engine.assets.particle({
		name = "game.sparks",
		texture = engine.assets.texture("particles/spark.png"),
		loop = false,
		duration = 0.1,
		bursts = { { count = 24 } },
		lifetime = { min = 0.2, max = 0.4 },
		speed = { min = 80, max = 160 },
		colorOverLife = { { t = 0, color = { 1, 0.9, 0.4 } }, { t = 1, color = { 1, 0.2, 0, 0 } } },
	}),
	trail = engine.assets.particle({
		name = "game.trail",
		texture = engine.assets.texture("particles/smoke.png"),
		blend = "alpha",
		rate = 40,
		lifetime = 0.6,
		angle = { min = math.pi - 0.2, max = math.pi + 0.2 },
		speed = 60,
		sizeOverLife = { { t = 0, value = 8 }, { t = 1, value = 24 } },
		colorOverLife = { { t = 0, color = { 1, 1, 1, 0.6 } }, { t = 1, color = { 1, 1, 1, 0 } } },
	}),
}
```

```luau
-- scripts/effects.luau: sparks where each new laser appears
if not engine.isClient then
	return
end

engine.ecs.system("game.MuzzleSparks", {
	phase = "Update",
	side = "client",
	query = { read = { "rtype.Transform" }, added = { "game.Laser" } },
	events = { send = { "rtype.SpawnParticles" } },
	run = function(ctx)
		for _, transform in ctx.query do
			local at = transform.position
			ctx.events.send("rtype.SpawnParticles", { effect = Game.effects.sparks, position = { x = at.x, y = at.y } })
		end
	end,
})
```

Units are the engine's: positions and sizes in world units (Y down), times in seconds, angles in radians (`0` points
right, along +X; `math.pi / 2` points down).

## `engine.assets.particle`

`engine.assets.particle({ ... })` builds an effect and returns its id. Only `name` is needed: every other key has a
default, and an effect with only a name emits nothing. The id is the one of the name, so `effect = "game.sparks"` in a
component means the same effect.

### Spawning

| Key | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | **required** | What the effect is known by |
| `rate` | number | `0` | Particles per second while the emitter plays |
| `bursts` | list | none | Particles emitted at once, see below |
| `duration` | number | `1` | Seconds of one cycle |
| `loop` | boolean | `true` | `true`: a new cycle starts after `duration`; `false`: the effect plays once |
| `capacity` | whole number | `1024` | Most particles alive at once; more are not spawned |
| `shape` | string or table | `"point"` | Where particles spawn, see below |

A burst is `{ count, time, cycles, interval }`: `count` particles (required) at `time` seconds into the cycle (default
`0`), `cycles` times (default `1`), `interval` seconds apart (default `0`). An explosion is a burst and `loop = false`;
a trail is a `rate` and `loop = true`.

The `shape` is `"point"`, `{ type = "circle", radius = 8 }` (`edgeOnly = true` spawns on the circle only, as a ring) or
`{ type = "rect", size = { x = 64, y = 4 } }`, centred on the emitter and turned and scaled with it.

### Each particle

A range is a number (always that value) or `{ min = , max = }`, picked per particle.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `lifetime` | range | `1` | Seconds; above 0 |
| `speed` | range | `0` | World units per second, along the angle |
| `angle` | range | `{ min = 0, max = 2 * math.pi }` | Direction it goes, in radians: every way by default |
| `rotation` | range | `0` | Of the quad, in radians, clockwise on screen |
| `angularVelocity` | range | `0` | Of the quad, in radians per second |
| `size` | range | `1` | Width and height of the quad, in world units |
| `color` | `{ r, g, b, a }` | `{ 1, 1, 1, 1 }` | Tints the texture; the alpha may be left out and is then 1 |
| `gravity` | `{ x, y }` | `{ x = 0, y = 0 }` | World units per second squared; +Y is down |
| `drag` | number | `0` | Slows particles down; `0` never does |

### Over a particle's life

Each list is a list of keys in order of `t`, from `0` (birth) to `1` (death). Between two keys the value goes in a
straight line; before the first and after the last it stays. Two keys at the same `t` make it jump.

| Key | One key | Meaning |
|---|---|---|
| `colorOverLife` | `{ t = , color = { r, g, b, a } }` | Multiplies `color`: fade the alpha to 0 to fade out |
| `sizeOverLife` | `{ t = , value = }` | Multiplies `size` |
| `speedOverLife` | `{ t = , value = }` | Multiplies the speed |

### How it looks

| Key | Type | Default | Meaning |
|---|---|---|---|
| `texture` | texture id | none | From [`engine.assets.texture`](setup-forms.md) |
| `source` | `{ position, size }` | the whole texture | The part of the texture to show, in pixels |
| `flipbook` | `{ columns, rows, frames, cycles }` | one frame | Frames of `source` in a grid, played over each life: `frames` is how many (`0`: every cell), `cycles` how many times |
| `blend` | string | `"additive"` | `"additive"` (glows: sparks, fire), `"alpha"` (smoke, dust) or `"cutout"` |
| `alignment` | string | `"fixed"` | `"fixed"`, or `"velocity"`: each quad turns along its motion (the texture's +X forward) |
| `stretch` | number | `0` | With `"velocity"`: the quad grows longer by speed × `stretch` |
| `material` | material id | the default one | From [`engine.assets.material`](setup-forms.md), for a shader of your own |
| `space` | string | `"world"` | `"world"`: particles stay where they spawned (trails); `"local"`: they follow the emitter (a shield's glow) |
| `sortByAge` | boolean | `false` | Draws the oldest first; always on with `"alpha"` |
| `simulation` | string | `"auto"` | `"auto"`, `"gpu"` or `"cpu"`: where particles are computed. Leave it to `"auto"` |

## Playing an effect

### Once, at a point

Send `rtype.SpawnParticles`: the plugin spawns an emitter there, plays its effect once and removes it.

| Field | Type | Default | Meaning |
|---|---|---|---|
| `effect` | effect id or name | none | The effect |
| `position` | `{ x, y }` | `{ x = 0, y = 0 }` | In world units |
| `layer` | integer | `0` | Draw order among the sprites |

Use an effect with `loop = false` here. A looping one never ends, so it would stay forever.

### On an entity

Give the entity an `rtype.ParticleEmitter` and an `rtype.Transform`. It plays while the component is there and moves
with the entity.

| Field | Type | Default | Meaning |
|---|---|---|---|
| `effect` | effect id or name | none | The effect; changing it restarts the emitter |
| `playing` | boolean | `true` | `false` stops emitting; living particles finish their life |
| `layer` | integer | `0` | Draw order among the sprites |
| `rateScale` | number | `1` | Multiplies the effect's `rate`: more smoke as a ship gets damaged |
| `seed` | whole number | `0` | The same seed gives the same particles; `0` picks one |
| `material` | material id | none | Replaces the effect's material |
| `overrideSimulation`, `simulation` | boolean, number | `false`, `0` | `true` replaces the effect's `simulation` with this one: `0` auto, `1` GPU, `2` CPU |

```luau
engine.ecs.system("game.ShipTrail", {
	phase = "Update",
	side = "client",
	query = { with = { "game.Ship" }, without = { "rtype.ParticleEmitter" } },
	run = function(ctx)
		for ship in ctx.query do
			ctx.commands:add(ship, "rtype.ParticleEmitter", { effect = Game.effects.trail, layer = -1 })
		end
	end,
})
```

The ship entity is replicated from the server, but `rtype.ParticleEmitter` is the client's own: a client system may
add it. Remove it, or set `playing = false`, to stop the trail.

### Knowing when it is over

`rtype.ParticlesFinished` (`entity`) is sent once an emitter whose effect does not loop has emitted everything and its
last particle has died. Read it under `events = { read = { "rtype.ParticlesFinished" } }`.

## Errors

The form checks its whole table and names the part that is wrong:

| Message | Cause |
|---|---|
| `engine.assets.particle: expected a 'name'` | No `name`, or an empty one |
| `engine.assets.particle: unexpected 'colour', expected ...` | A key the form does not know |
| `engine.assets.particle: 'lifetime': expected a number or a table { min = , max = }` | A range of the wrong type |
| `engine.assets.particle: 'color': expected a colour { r, g, b } or { r, g, b, a }` | A colour with too few or too many numbers |
| `engine.assets.particle: burst 1: expected a 'count'` | A burst without its `count` |
| `engine.assets.particle: 'sizeOverLife' key 1: expected a 't'` | A key without its `t` |
| `... Unknown blend 'glow' (expected 'additive', 'alpha', 'cutout')` | A name of no known choice, for `blend`, `alignment`, `space`, `simulation` or the `shape` type |
| `... Particle effect 'a': ...` | The effect is invalid, the reason follows: a `min` above its `max`, a `lifetime` of 0, keys out of order or outside 0 to 1 |

Building an effect twice with the same `name` keeps the first one.

!!! warning "Only on the client"
    `rtype.ParticleEmitter`, `rtype.SpawnParticles` and `rtype.ParticlesFinished` exist only where `rtype.Particle`
    is, on the client. Declare the systems that name them with `side = "client"`, in a file that starts with
    `if not engine.isClient then return end` (see [Systems](systems.md#where-a-system-runs)). Keep
    `engine.assets.particle` outside that file, so that the server has the ids too.

## See also

- [Setup forms](setup-forms.md): textures and materials an effect uses.
- [Systems](systems.md): events, commands and sides.
- [Components](components.md): the engine's components and client-only data.
