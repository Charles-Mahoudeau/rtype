-- The boosts' effects, from each rider's game.Boosted and game.Shield (assets/particles/):
--   speed    small arrows rising slowly from the rider and fading, while it lasts;
--   points   a burst of twinkles that grow and shrink, flicker and fade, when it is won;
--   shield   a bubble breathing around the rider and shading white, green, white, green, while it lasts;
--   sniper   a red scope over the rider, steady, pulsing faster.
-- A lasting effect is an emitter entity of its own (game.BoostFx), spawned when the boost starts and following its
-- rider; when the boost ends the speed's arrows stop and those in the air live on, the others go at once.

local Game = require("@scripts/shared/rules")

local function texture(path: string)
	return engine.assets.texture(path, { linear = false })
end

local riderHeight = Game.riderSize.y

local speedEffect = engine.assets.particle({
	name = "game.speedBoost",
	texture = texture("particles/boost.png"),
	blend = "alpha",
	rate = 14,
	shape = { type = "rect", size = { x = Game.riderSize.x, y = riderHeight } }, -- all over the rider
	lifetime = { min = 0.9, max = 1.4 },
	speed = { min = 50, max = 90 }, -- slowly, up the view
	angle = { min = -math.pi / 2 - 0.12, max = -math.pi / 2 + 0.12 },
	size = { min = 19, max = 32 },
	colorOverLife = {
		{ t = 0, color = { 1, 1, 1, 0 } },
		{ t = 0.15, color = { 1, 1, 1, 1 } },
		{ t = 1, color = { 1, 1, 1, 0 } },
	},
	space = "world", -- they stay where they rose, the rider rides on
})
-- How long the arrows in the air still live once the boost ends: their longest life.
local speedLinger = 1.4

local twinkleEffect = engine.assets.particle({
	name = "game.pointsBoost",
	texture = texture("particles/twinkle.png"),
	blend = "alpha",
	loop = false,
	duration = 0.05,
	bursts = { { count = 20 } }, -- all at once
	shape = { type = "circle", radius = 24 },
	lifetime = { min = 0.9, max = 1.5 }, -- lives of different lengths: they do not twinkle in step
	speed = { min = 40, max = 160 },
	drag = 2,
	size = { min = 29, max = 51 },
	-- Bigger, smaller, bigger, smaller, then gone; more transparent at each low, then faded out.
	sizeOverLife = {
		{ t = 0, value = 0.4 },
		{ t = 0.15, value = 1.2 },
		{ t = 0.3, value = 0.6 },
		{ t = 0.45, value = 1.2 },
		{ t = 0.6, value = 0.6 },
		{ t = 0.75, value = 1 },
		{ t = 1, value = 0.2 },
	},
	colorOverLife = {
		{ t = 0, color = { 1, 1, 1, 1 } },
		{ t = 0.15, color = { 1, 1, 1, 0.45 } },
		{ t = 0.3, color = { 1, 1, 1, 1 } },
		{ t = 0.45, color = { 1, 1, 1, 0.45 } },
		{ t = 0.6, color = { 1, 1, 1, 1 } },
		{ t = 0.75, color = { 1, 1, 1, 0.5 } },
		{ t = 1, color = { 1, 1, 1, 0 } },
	},
})

local white = { 1, 1, 1, 0.9 }
local green = { 0.3, 1, 0.4, 0.9 }
local shieldEffect = engine.assets.particle({
	name = "game.shieldBoost",
	texture = texture("particles/shield.png"),
	blend = "alpha",
	duration = 1,
	bursts = { { count = 1 } }, -- one a second, each living about two: two bubbles at different moments of their life
	lifetime = { min = 1.8, max = 2.4 },
	size = { min = riderHeight * 1.2, max = riderHeight * 1.35 }, -- each its own size: an uneven breath
	sizeOverLife = { { t = 0, value = 0.92 }, { t = 0.5, value = 1.06 }, { t = 1, value = 0.92 } },
	colorOverLife = {
		{ t = 0, color = { 1, 1, 1, 0 } },
		{ t = 0.12, color = white },
		{ t = 0.37, color = green },
		{ t = 0.62, color = white },
		{ t = 0.87, color = green },
		{ t = 1, color = { 0.3, 1, 0.4, 0 } },
	},
	space = "local", -- it stays around the rider
})

-- The scope: one at a time, a cycle each living exactly a cycle, the same size and alpha at both ends: a steady pulse.
local scopePulse = 0.45 -- seconds
local scopeEffect = engine.assets.particle({
	name = "game.sniperBoost",
	texture = texture("particles/scope.png"),
	blend = "alpha",
	duration = scopePulse,
	bursts = { { count = 1 } },
	lifetime = scopePulse,
	size = riderHeight * 1.1,
	color = { 1, 0.25, 0.25, 1 }, -- red only
	sizeOverLife = { { t = 0, value = 1 }, { t = 0.5, value = 1.1 }, { t = 1, value = 1 } },
	colorOverLife = {
		{ t = 0, color = { 1, 1, 1, 0.75 } },
		{ t = 0.5, color = { 1, 1, 1, 1 } },
		{ t = 1, color = { 1, 1, 1, 0.75 } },
	},
	space = "local",
})

-- The lasting effects, by boost, and their draw layer: over the riders (Game.riderDrawLayer), whom the bubble and the
-- scope let through.
local lasting = {
	[Game.speedBoost] = { effect = speedEffect, layer = 2 },
	[Game.shieldBoost] = { effect = shieldEffect, layer = 2 },
	[Game.sniperBoost] = { effect = scopeEffect, layer = 2 },
}
local twinkleLayer = 3

-- This frame's riders, by their bits: where each is and which lasting boosts it has on (filled by game.ReadBoosts).
local riderAt: { [number]: { x: number, y: number } } = {}
local boostsOn: { [number]: { [number]: boolean } } = {}
-- The points boosts each rider had won, to twinkle a new one; and the emitter of each lasting effect playing.
local pointsOf: { [number]: number } = {}
local emitterOf: { [number]: { [number]: any } } = {}

-- Update: reads each rider's boosts, twinkles a new points boost, and spawns the emitter of a lasting boost that
-- started.
engine.ecs.system("game.ReadBoosts", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Position", "game.Boosted", "game.Shield" }, with = { "game.Rider" } },
	events = { send = { "rtype.SpawnParticles" } },
	after = { "game.SpawnEffects" },
	run = function(ctx)
		table.clear(riderAt)
		for entity, position, boosted, shield in ctx.query do
			local bits = entity.bits
			local at = position.value
			riderAt[bits] = at
			local on = boostsOn[bits] or {}
			boostsOn[bits] = on
			on[Game.speedBoost] = boosted.speed
			on[Game.shieldBoost] = shield.active and not shield.ghost
			on[Game.sniperBoost] = boosted.sniper

			local won = pointsOf[bits]
			if won ~= nil and won ~= boosted.points then
				ctx.events.send("rtype.SpawnParticles", { effect = twinkleEffect, position = at, layer = twinkleLayer })
			end
			pointsOf[bits] = boosted.points

			local emitters = emitterOf[bits] or {}
			emitterOf[bits] = emitters
			for boost, look in lasting do
				if on[boost] and emitters[boost] == nil then
					emitters[boost] = ctx.commands:spawn({
						["game.BoostFx"] = { rider = entity, boost = boost },
						["rtype.Transform"] = { position = at },
						["rtype.ParticleEmitter"] = { effect = look.effect, layer = look.layer },
					})
				end
			end
		end
		for bits in boostsOn do
			if riderAt[bits] == nil then -- the rider left
				boostsOn[bits], pointsOf[bits], emitterOf[bits] = nil, nil, nil
			end
		end
	end,
})

-- Update, once everything moved: each emitter follows its rider. When its boost ends, the arrows stop and go once the
-- last ones faded; another effect, or one whose rider left, goes at once.
engine.ecs.system("game.MoveBoostFx", {
	phase = "Update",
	side = "client",
	query = { write = { "game.BoostFx", "rtype.Transform" } },
	after = { "game.ReadBoosts", "game.ShakeCamera" },
	run = function(ctx)
		for entity, fx, transform in ctx.query do
			local riderBits = fx.rider.bits
			local at = riderAt[riderBits]
			if fx.left > 0 then -- stopped: its last particles live on
				fx.left -= ctx.time.delta
				if fx.left <= 0 then
					ctx.commands:despawn(entity)
				end
				continue
			end
			local on = boostsOn[riderBits]
			if at ~= nil and on ~= nil and on[fx.boost] then
				transform.position = at
				continue
			end
			local emitters = emitterOf[riderBits]
			if emitters ~= nil then
				emitters[fx.boost] = nil -- a new one starts with the boost's next time
			end
			if at ~= nil and fx.boost == Game.speedBoost then
				ctx.commands:patch(entity, "rtype.ParticleEmitter", { playing = false })
				fx.left = speedLinger
			else
				ctx.commands:despawn(entity)
			end
		end
	end,
})
