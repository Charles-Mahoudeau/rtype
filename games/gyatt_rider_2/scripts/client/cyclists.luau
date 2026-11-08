-- The cyclists' boosts, shown by their color: the sprite of a cyclist that carries one is tinted, so that the riders
-- know which to shoot.

local Game = require("@scripts/shared/rules")

local tints = {
	[Game.pointsBoost] = { x = 1, y = 0.85, z = 0.2, w = 1 }, -- gold: points
	[Game.speedBoost] = { x = 0.3, y = 0.9, z = 1, w = 1 }, -- cyan: speed
	[Game.shieldBoost] = { x = 0.4, y = 1, z = 0.4, w = 1 }, -- green: invincibility
	[Game.sniperBoost] = { x = 0.7, y = 0.08, z = 0.08, w = 1 }, -- dark red: sniper shots, darker than the riders
}

-- Update, once the cyclist has its sprite (client/looks.luau): tints the cyclists that carry a boost, once.
engine.ecs.system("game.TintCyclists", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Cyclist" }, write = { "rtype.SpriteRenderer" }, added = { "rtype.SpriteRenderer" } },
	after = { "game.DressLooks", "game.TwinkleShields" },
	run = function(ctx)
		for _, cyclist, sprite in ctx.query do
			local tint = tints[cyclist.boost]
			if tint then
				sprite.color = tint
			end
		end
	end,
})
