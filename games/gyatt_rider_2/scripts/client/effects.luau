-- The particles and the screen shake: a burst of squares where each beam stops, a bigger one on each rider hit, and
-- the camera shaking when the player's own rider is hit. The particles are a plain white square (assets/particle.png),
-- tinted: square, hard-edged quads.

local Game = require("@scripts/shared/rules")
local RiderEvents = require("@scripts/client/riderevents")

local square = engine.assets.texture("particle.png", { linear = false })

local impact = engine.assets.particle({
	name = "game.impact",
	texture = square,
	loop = false,
	duration = 0.1,
	bursts = { { count = 18 } },
	lifetime = { min = 0.15, max = 0.3 },
	speed = { min = 120, max = 320 },
	size = { min = 6, max = 10 },
	color = { 1, 0.75, 0.2 },
	colorOverLife = { { t = 0, color = { 1, 1, 1, 1 } }, { t = 1, color = { 1, 0.4, 0, 0 } } },
	sizeOverLife = { { t = 0, value = 1 }, { t = 1, value = 0.3 } },
})
local hitBurst = engine.assets.particle({
	name = "game.hit",
	texture = square,
	loop = false,
	duration = 0.1,
	bursts = { { count = 36 } },
	lifetime = { min = 0.3, max = 0.6 },
	speed = { min = 80, max = 380 },
	size = { min = 8, max = 14 },
	drag = 2,
	colorOverLife = {
		{ t = 0, color = { 1, 1, 1, 1 } },
		{ t = 0.3, color = { 1, 0.2, 0.2, 1 } },
		{ t = 1, color = { 0.6, 0, 0, 0 } },
	},
})
local effectsLayer = 3 -- over the riders

local shakeSeconds = 0.35
local shakeStrength = 14 -- world units, at the start of a shake
local shakeLeft = 0 -- seconds of shaking left

-- Update: a burst where each of this frame's beams stops, and one on each rider hit; the player's own hit shakes.
engine.ecs.system("game.SpawnEffects", {
	phase = "Update",
	side = "client",
	query = {},
	events = { send = { "rtype.SpawnParticles" } },
	after = { "game.DetectRiderEvents" },
	run = function(ctx)
		for _, shot in RiderEvents.shots() do
			ctx.events.send("rtype.SpawnParticles", { effect = impact, position = shot.to, layer = effectsLayer })
		end
		for _, hit in RiderEvents.hits() do
			ctx.events.send("rtype.SpawnParticles", { effect = hitBurst, position = hit.at, layer = effectsLayer })
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
