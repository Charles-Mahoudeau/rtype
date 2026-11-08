-- The shields: a ghost (a rider just hit, whose game.Shield is on as a ghost) twinkles, its sprite fading in and out,
-- so that everyone sees it cannot be hit nor shoot (with less motion in the settings, it stays faded instead); it
-- turns solid again when the shield turns off. A rider invincible by the boost does not twinkle: its own effect
-- shows it (client/boostfx.luau).

local Settings = require("@scripts/client/settings")

local twinklesPerSecond = 10
local faded = 0.25 -- the sprite's alpha at the low of a twinkle
local steady = 0.5 -- the sprite's alpha of a ghost that does not twinkle

-- Update, once the rider has its sprite (client/looks.luau): sets the alpha of each rider's sprite from its shield.
engine.ecs.system("game.TwinkleShields", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Shield" }, write = { "rtype.SpriteRenderer" }, with = { "game.Rider" } },
	after = { "game.DressLooks" },
	run = function(ctx)
		local ghostAlpha = if Settings.get("reduceMotion")
			then steady
			elseif math.floor(ctx.time.elapsed * twinklesPerSecond * 2) % 2 == 1 then faded
			else 1
		for _, shield, sprite in ctx.query do
			local alpha = if shield.active and shield.ghost then ghostAlpha else 1
			local color = sprite.color
			if color.w ~= alpha then
				sprite.color = { x = color.x, y = color.y, z = color.z, w = alpha }
			end
		end
	end,
})
