-- The riders: client/looks.luau draws every rider with the cyclist's white image, tinted here in its player's color
-- (the HUD's, by slot), the same on every client. The player's own rider pedals faster than the others, so that it
-- tells itself apart at a glance.

local Hud = require("@scripts/client/hud")

local ownPedaling = 1.5 -- times the riding animation's speed
local otherPedaling = 0.4

-- Update, once the riders have their sprites and animations (client/looks.luau): tints each rider in its player's
-- color, keeping the alpha the shield's twinkle sets, and sets how fast it pedals. Written only when they change.
engine.ecs.system("game.TintRiders", {
	phase = "Update",
	side = "client",
	query = {
		read = { "game.Rider", "net.Owner" },
		write = { "rtype.SpriteRenderer", "rtype.SpriteAnimator" },
	},
	calls = { "rtype.replication.localClient" },
	after = { "game.DressLooks" },
	before = { "game.TwinkleShields" },
	run = function(ctx)
		local me = ctx.call("rtype.replication.localClient", {})
		for _, rider, owner, sprite, animator in ctx.query do
			local tint = Hud.colorOf(rider.slot)
			local color = sprite.color
			if color.x ~= tint[1] or color.y ~= tint[2] or color.z ~= tint[3] then
				sprite.color = { x = tint[1], y = tint[2], z = tint[3], w = color.w }
			end
			local pedaling = if me.connected and owner.client == me.client then ownPedaling else otherPedaling
			if animator.speed ~= pedaling then
				animator.speed = pedaling
			end
		end
	end,
})
