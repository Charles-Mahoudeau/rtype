-- The particles and the screen shake. One effect, the impact (assets/particles/impact.png): a single particle the
-- size of a rider's width that grows and fades out, at the start of each beam, where a beam stops on something, and
-- on each rider hit. The camera shakes when the player's own rider is hit.

local Game = require("@scripts/shared/rules")
local RiderEvents = require("@scripts/client/riderevents")

local impact = engine.assets.particle({
	name = "game.impact",
	texture = engine.assets.texture("particles/impact.png", { linear = false }),
	blend = "alpha", -- the image's own colors
	loop = false,
	duration = 0.05,
	bursts = { { count = 1 } },
	lifetime = 0.35,
	size = Game.riderSize.x, -- square, as wide as a rider
	sizeOverLife = { { t = 0, value = 1 }, { t = 1, value = 2 } },
	color = { 1, 1, 1, 0.5 },
	colorOverLife = { { t = 0, color = { 1, 1, 1, 1 } }, { t = 1, color = { 1, 1, 1, 0 } } },
})
local effectsLayer = 3 -- over the riders

--- Whether a beam that ends at @p to stopped on something, rather than at the view's edge.
local function stoppedOn(to): boolean
	return to.x > Game.roadsLeft and to.x < Game.roadsRight and to.y > 0 and to.y < Game.viewHeight
end

local shakeSeconds = 0.35
local shakeStrength = 14 -- world units, at the start of a shake
local shakeLeft = 0 -- seconds of shaking left

-- Update: an impact at the start of each of this frame's beams, where it stopped on something, and on each rider hit;
-- the player's own hit shakes.
engine.ecs.system("game.SpawnEffects", {
	phase = "Update",
	side = "client",
	query = {},
	events = { send = { "rtype.SpawnParticles" } },
	after = { "game.DetectRiderEvents" },
	run = function(ctx)
		local function burst(at)
			ctx.events.send("rtype.SpawnParticles", { effect = impact, position = at, layer = effectsLayer })
		end
		for _, shot in RiderEvents.shots() do
			burst(shot.from)
			if stoppedOn(shot.to) then
				burst(shot.to)
			end
		end
		for _, hit in RiderEvents.hits() do
			burst(hit.at)
			if hit.mine then
				shakeLeft = shakeSeconds
			end
		end
	end,
})

-- Update, last of what moves things: the camera shakes around the view's middle while a shake lasts, weaker and weaker.
engine.ecs.system("game.ShakeCamera", {
	phase = "Update",
	side = "client",
	query = { write = { "rtype.Transform" }, with = { "game.Camera" } },
	after = { "game.SpawnEffects", "game.MoveGaugeDots", "game.FillGauge", "game.DriveLooks" },
	run = function(ctx)
		if shakeLeft <= 0 then
			return
		end
		shakeLeft = math.max(0, shakeLeft - ctx.time.delta)
		local strength = shakeStrength * shakeLeft / shakeSeconds
		local middle = { x = Game.viewWidth / 2, y = Game.viewHeight / 2 }
		for _, transform in ctx.query do
			transform.position = {
				x = middle.x + (math.random() * 2 - 1) * strength,
				y = middle.y + (math.random() * 2 - 1) * strength,
			}
		end
	end,
})
