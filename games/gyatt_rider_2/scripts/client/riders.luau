-- The riders: client/looks.luau draws every rider with the cyclist's white image, tinted here in its player's color
-- (the HUD's, by slot), the same on every client. The player's own rider pedals faster than the others, so that it
-- tells itself apart at a glance. Every second frame of the ride (1 2 3 2 1...: the middle one, 2) is a bit brighter.

local Hud = require("@scripts/client/hud")
local assets = require("@scripts/client/assets")

local ownPedaling = 1.5 -- times the riding animation's speed
local otherPedaling = 0.4
local brighter = 0.9 -- how far toward white the middle frame's color goes

--- A rider's color on this frame of its ride: its player's, a bit brighter on every second frame.
local function colorOf(slot: number, time: number): { number }
	local tint = Hud.colorOf(slot)
	if math.floor(time / assets.rideFrameSeconds) % 2 == 1 then
		for channel = 1, 3 do
			tint[channel] += (1 - tint[channel]) * brighter
		end
	end
	return tint
end

-- Update, once the riders have their sprites and animations (client/looks.luau): tints each rider in its player's
-- color, brighter on the middle frame, keeping the alpha the shield's twinkle sets, and sets how fast it pedals.
-- Written only when they change.
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
			local tint = colorOf(rider.slot, animator.time)
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
