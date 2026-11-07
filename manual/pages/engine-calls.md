# Engine calls

Plain components cover most of a game. An engine call is for what data cannot express: loading an asset, binding an
input, playing a sound, asking the physics a question. It is a function the engine gives to scripts, with a name
`rtype.<feature>.<verb>`, called with a table of arguments.

```luau
-- while the script loads (top level of a file)
local sound = engine.call("rtype.audio.loadSound", { path = "laser.mp3" })

-- in a system: declare it, then call it
engine.ecs.system("game.Pew", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "rtype.audio.playSound" },
	run = function(ctx)
		ctx.call("rtype.audio.playSound", { sound = sound, volume = 0.9 })
	end,
})
```

- Arguments you leave out keep their defaults. An `assetId` argument accepts a path.
- A text argument or result is a Luau string. Short names (a sound, an action) hold 255 bytes at most and a longer
  one is cut; a call that takes or gives a long text (a prompt, a request, a reply) takes it whole, at any length,
  as its table says. A list argument is a Luau array.
- A result of one field is returned as that value, a result of several as a table, a call with no result returns
  nothing.
- A system that calls something it did not list under `calls` gets a Luau error. An unknown call is refused when the
  script loads, except in a system that does not run on that side (a client system may list a call that only the client
  has).
- An unknown name, a wrong argument or a failing call is a Luau error that names the call.
- A call exists only where its feature is: a server has no textures or sounds. The [setup forms](setup-forms.md) hide
  this for assets.

## The calls

### Sprites

An atlas is made with one call and filled by name with the others.

| Call | Arguments | Result |
|---|---|---|
| `rtype.sprite.loadAtlas` | `path` (an Aseprite or TexturePacker JSON) | the atlas id |
| `rtype.sprite.createAtlas` | `name`, `texture`, and for a grid `frameSize`, `count`, `spacing` | the atlas id |
| `rtype.sprite.addFrame` | `atlas`, `position`, `size` | the frame's index |
| `rtype.sprite.addAnimation` | `atlas`, `name`, `frameDuration`, `direction`, a range of frames | nothing |

Prefer the `engine.assets.atlas` form, which makes these calls from one table.

### Textures, fonts, maps

| Call | Arguments | Result |
|---|---|---|
| `rtype.backend.loadTexture` | `path` | the texture id |
| `rtype.text.loadFont` | `path` (a font atlas JSON) | the font id |
| `rtype.tilemap.load` | `path` (a Tiled `.tmj`) | the map id |
| `rtype.tilemap.spawn` | `map`, `position`, `scale`, `firstDrawLayer` | how many layers it spawned |

Texts are spawned as components:

```luau
ctx.commands:spawn({
	["rtype.Transform"] = { position = { x = 400, y = 12 } },
	["rtype.TextRenderer"] = { text = "Player 1", font = font, size = 22, anchor = { x = 0.5, y = 0 } },
})
```

### Audio

| Call | Arguments | Result |
|---|---|---|
| `rtype.audio.loadSound` | `path`, `stream` (decode while playing, for music) | the sound id |
| `rtype.audio.playSound` | `sound`, `bus` (`"effects"` or `"music"`), `volume`, `pitch`, `position` | nothing |

A `position` left out plays the sound centred. Music is started by sending the `rtype.PlayMusic` event
(`{ sound, crossfade }`), and stopped with `rtype.StopMusic`.

### Input

Controls are named with text: a key without its prefix (`"W"`, `"Space"`, `"Up"`, `"LeftShift"`), `"Mouse"` plus a
button (`"MouseLeft"`), `"MouseDeltaX"`, `"MouseScrollY"`, `"Gamepad"` plus a button or axis (`"GamepadSouth"`,
`"GamepadLeftX"`). Keys are physical positions named after the US layout, so WASD lands on ZQSD on a French keyboard.

| Call | Arguments | Result |
|---|---|---|
| `rtype.input.addAction` | `name`, `type` (`"button"`, `"axis"`, `"vector2"`) | nothing |
| `rtype.input.bind` | `action`, `control`, optional `scale` | nothing |
| `rtype.input.bindAxis` | `action`, `negative`, `positive` | nothing |
| `rtype.input.bindVector` | `action`, `up`, `down`, `left`, `right` or `x`, `y` | nothing |
| `rtype.input.read` | `action` | `{ value, held, pressed, released }` |

Prefer `engine.input.action`, see [Setup forms](setup-forms.md). To read an action on the server, see
[Networking](networking.md).

### Physics

See [Physics](physics.md).

### Capsules

| Call | Arguments | Result |
|---|---|---|
| `rtype.capsule.spawn` | `capsule` (a path) | the root entity |
| `rtype.capsule.load` | `capsule` (a path): switches levels | the root entity |
| `rtype.capsule.reload` | `root` | `true` if it was a capsule's root |

See [Capsules](capsules.md).

### Replication

| Call | Where | What |
|---|---|---|
| `rtype.replication.addInput` | both | declares a network input (made for you by `engine.input.action`) |
| `rtype.replication.readInput` | server | the input of a player's action, see [Networking](networking.md) |
| `rtype.replication.localClient` | client | `{ connected, client }`: whether the server gave this client its id, and the id (as `net.Owner` and `net.ClientConnected` carry it) |

### Shaders and particles

The shader plugin has `engine.assets.shader` and `engine.assets.material` forms, see [Setup forms](setup-forms.md).
Particle effects have the `engine.assets.particle` form, see [Particles](particles.md). It is made of the calls
`rtype.particle.beginEffect`, `rtype.particle.addBurst`, `rtype.particle.addColorKey`, `rtype.particle.addCurveKey` and
`rtype.particle.addEffect` (client); use the form rather than the calls.
