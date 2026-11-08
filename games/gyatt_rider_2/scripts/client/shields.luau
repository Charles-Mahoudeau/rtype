-- The shields: a rider whose game.Shield is on twinkles, its sprite fading in and out, so that everyone sees it cannot
-- be hit; it turns solid again when the shield turns off.

local twinklesPerSecond = 10
local faded = 0.25 -- the sprite's alpha at the low of a twinkle

-- Update, once the rider has its sprite (client/looks.luau): sets the alpha of each rider's sprite from its shield.
engine.ecs.system("game.TwinkleShields", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Shield" }, write = { "rtype.SpriteRenderer" }, with = { "game.Rider" } },
	after = { "game.DressLooks" },
	run = function(ctx)
		local low = math.floor(ctx.time.elapsed * twinklesPerSecond * 2) % 2 == 1
		for _, shield, sprite in ctx.query do
			local alpha = if shield.active and low then faded else 1
			local color = sprite.color
			if color.w ~= alpha then
				sprite.color = { x = color.x, y = color.y, z = color.z, w = alpha }
			end
		end
	end,
})
