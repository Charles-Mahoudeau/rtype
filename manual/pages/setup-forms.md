# Setup forms

Every game declares its assets and its input actions. The setup forms do it from one readable table each, and the same
script runs on the server and the clients: a side that does not have the feature (a server has no textures) skips the
loading and still gives you the asset's id, so the rest of the script does not care.

A form checks its whole table on every side. An unknown key, a value of the wrong type or a binding of no known shape is
an error that names the form and the part of it (`engine.assets.atlas: frame 2: 'size': missing 'y'`).

## `engine.assets`

Paths are relative to the game's `assets/` folder and cannot leave it. Each form returns the asset's id, to keep in a
table and use in components and calls.

```luau
Game.assets = {
	ship = engine.assets.atlas({
		name = "game.ship",
		texture = "r-typesheet24.gif",
		grid = { frameSize = { x = 48, y = 48 }, count = 5, spacing = { x = 17, y = 0 }, offset = { x = 9, y = 9 } },
		animations = { fly = { frames = { 0, 1, 2, 3, 4 }, frameDuration = 0.1 } },
	}),
	font = engine.assets.font("fonts/orbitron.json"),
	music = engine.assets.sound("maint.ogg", { stream = true }),
	shot = engine.assets.sound("laser.mp3"),
	background = engine.assets.tilemap("levels/space-background.tmj"),
	planet = engine.assets.texture("backgrounds/space/parallax-space-big-planet.png"),
}
```

| Form | Returns | Notes |
|---|---|---|
| `engine.assets.texture(path, { linear, wrap })` | texture id | |
| `engine.assets.sound(path, { stream })` | sound id | `stream = true` decodes while playing, for music |
| `engine.assets.font(path)` | font id | a font atlas made with msdf-atlas-gen (`.json` next to its `.png`) |
| `engine.assets.tilemap(path)` | map id | a Tiled `.tmj` |
| `engine.assets.shader(path)` | shader id | written in the engine's GLSL dialect |
| `engine.assets.material({ name, shader, values, textures })` | material id | a shader and its values |
| `engine.assets.atlas({ path \| texture, name, linear, grid, frames, animations })` | atlas id | see below |

### Atlases

An atlas names the frames of an image and the animations over them. Make it from a `path` to an atlas file (Aseprite
or TexturePacker JSON), or from a `texture` with a `grid` or a list of `frames`.

- `grid`: `frameSize` and `count` (both needed), `spacing`, `columns`, `offset`.
- `frames`: a list of `{ position, size, name?, origin? }`. A frame's name is its index when left out; its origin is its
  centre.
- `animations`: a table of animation name to `{ frames = {0, 1, 2} }` (consecutive frame indices, counted from 0) or
  `{ first, count }`, plus `frameDuration`, `direction` (for example `"pingPong"`) and `plays` (`0`: forever).
- `grid`, `frames` and `name` make an atlas from a `texture`, not from a `path`.

An atlas gives an entity its look through the `rtype.SpriteRenderer` component; an `rtype.SpriteAnimator` plays an
animation.

## `engine.input`

```luau
engine.input.action("move", "vector2", {
	bind = {
		{ up = "W", down = "S", left = "A", right = "D" },
		{ up = "Up", down = "Down", left = "Left", right = "Right" },
	},
})
engine.input.action("steer", "axis", { bind = { { negative = "Left", positive = "Right" } } })
engine.input.action("shoot", "button", { bind = { "MouseLeft", { control = "GamepadSouth", scale = 1 } } })
```

`engine.input.action(name, type, { bind })` declares an action. `type` is `"button"` (default), `"axis"` or
`"vector2"`. A binding is:

| Binding | Meaning |
|---|---|
| `"Space"` | a control by name |
| `{ control, scale }` | a control, scaled |
| `{ negative, positive }` | two controls making an axis |
| `{ up, down, left, right }` or `{ x, y }` | four controls or two stick axes making a vector |

The names of controls are in [Engine calls](engine-calls.md#input). In a networked game the action is also declared in
the network input layout on both sides, and the client sends its state to the server each tick. See
[Networking](networking.md).
