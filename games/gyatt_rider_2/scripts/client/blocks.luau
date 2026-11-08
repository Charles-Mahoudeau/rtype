-- Plain rectangles drawn as sprites: assets/white.png tinted and stretched. A shape (rtype.ShapeRenderer) draws over
-- every sprite whatever its layer, so the strip on the left and its gauge are blocks, which the gauge's small cyclists
-- can ride over.

local assets = require("@scripts/client/assets")

local side = 4 -- pixels of assets/white.png

return {
	--- The rtype.SpriteRenderer of a block of @p color ({ x, y, z, w }) on draw layer @p layer, centred on its position.
	sprite = function(color, layer: number)
		return {
			texture = assets.white,
			source = { x = 0, y = 0, z = side, w = side },
			origin = { x = side / 2, y = side / 2 },
			color = color,
			layer = layer,
		}
	end,
	--- The rtype.Transform's scale that makes a block @p size ({ x, y }) world units big.
	scale = function(size)
		return { x = size.x / side, y = size.y / side }
	end,
}
