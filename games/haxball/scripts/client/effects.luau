-- The particles: a flash where the ball is kicked, sparks where it hits a wall, confetti in the scorer's colors on a
-- goal, a trail behind a fast ball and dust under running players. The server tells what happened with packets; the
-- trail and the dust follow the bodies' speed.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/client/state")
local assets = require("@scripts/client/assets")

local dustDrawLayer = 1 -- under the players
local trailDrawLayer = 1 -- under the ball
local burstDrawLayer = 6 -- over everything on the pitch
local trailSpeed = { from = 150, to = 600 } -- the trail starts at the first speed and is full at the second
local dustSpeed = { from = 60, to = 200 }

local function fade(r: number, g: number, b: number, alpha: number): { any }
	return { { t = 0, color = { r, g, b, alpha } }, { t = 1, color = { r, g, b, 0 } } }
end

local effects = {
	kick = engine.assets.particle({
		name = "game.kick",
		texture = assets.dot,
		loop = false,
		duration = 0.1,
		bursts = { { count = 16 } },
		lifetime = { min = 0.18, max = 0.35 },
		speed = { min = 120, max = 260 },
		drag = 4,
		size = { min = 4, max = 8 },
		colorOverLife = fade(1, 0.95, 0.7, 1),
	}),
	kickWave = engine.assets.particle({
		name = "game.kickWave",
		texture = assets.ring,
		loop = false,
		duration = 0.1,
		bursts = { { count = 1 } },
		lifetime = 0.3,
		size = 64,
		sizeOverLife = { { t = 0, value = 0.3 }, { t = 1, value = 1.2 } },
		colorOverLife = fade(1, 1, 1, 0.8),
	}),
	bounce = engine.assets.particle({
		name = "game.bounce",
		texture = assets.dot,
		loop = false,
		duration = 0.1,
		bursts = { { count = 8 } },
		lifetime = { min = 0.15, max = 0.3 },
		speed = { min = 50, max = 140 },
		drag = 3,
		size = { min = 3, max = 6 },
		colorOverLife = fade(0.9, 1, 0.9, 0.8),
	}),
	-- The confetti of each team, white and gold stars for both.
	confetti = {
		[Game.red] = engine.assets.particle({
			name = "game.confettiRed",
			texture = assets.confetti,
			blend = "alpha",
			loop = false,
			duration = 0.6,
			bursts = { { count = 70 }, { count = 40, time = 0.3 } },
			lifetime = { min = 1.4, max = 2.4 },
			speed = { min = 180, max = 520 },
			drag = 1.6,
			gravity = { x = 0, y = 220 },
			size = { min = 8, max = 14 },
			rotation = { min = 0, max = 2 * math.pi },
			angularVelocity = { min = -10, max = 10 },
			color = { 1, 0.35, 0.3 },
			colorOverLife = { { t = 0, color = { 1, 1, 1, 1 } }, { t = 0.8, color = { 1, 1, 1, 1 } }, { t = 1, color = { 1, 1, 1, 0 } } },
		}),
		[Game.blue] = engine.assets.particle({
			name = "game.confettiBlue",
			texture = assets.confetti,
			blend = "alpha",
			loop = false,
			duration = 0.6,
			bursts = { { count = 70 }, { count = 40, time = 0.3 } },
			lifetime = { min = 1.4, max = 2.4 },
			speed = { min = 180, max = 520 },
			drag = 1.6,
			gravity = { x = 0, y = 220 },
			size = { min = 8, max = 14 },
			rotation = { min = 0, max = 2 * math.pi },
			angularVelocity = { min = -10, max = 10 },
			color = { 0.35, 0.6, 1 },
			colorOverLife = { { t = 0, color = { 1, 1, 1, 1 } }, { t = 0.8, color = { 1, 1, 1, 1 } }, { t = 1, color = { 1, 1, 1, 0 } } },
		}),
	},
	stars = engine.assets.particle({
		name = "game.stars",
		texture = assets.star,
		loop = false,
		duration = 0.6,
		bursts = { { count = 30 }, { count = 20, time = 0.3 } },
		lifetime = { min = 0.8, max = 1.6 },
		speed = { min = 120, max = 420 },
		drag = 2,
		gravity = { x = 0, y = 120 },
		size = { min = 8, max = 18 },
		angularVelocity = { min = -6, max = 6 },
		colorOverLife = { { t = 0, color = { 1, 0.9, 0.5, 1 } }, { t = 1, color = { 1, 1, 1, 0 } } },
	}),
	trail = engine.assets.particle({
		name = "game.ballTrail",
		texture = assets.dot,
		blend = "alpha",
		rate = 140,
		lifetime = 0.25,
		size = 20,
		sizeOverLife = { { t = 0, value = 1 }, { t = 1, value = 0.3 } },
		colorOverLife = fade(1, 1, 1, 0.5),
	}),
	dust = engine.assets.particle({
		name = "game.dust",
		texture = assets.smoke,
		blend = "alpha",
		rate = 14,
		shape = { type = "circle", radius = 10 },
		lifetime = { min = 0.35, max = 0.6 },
		speed = { min = 5, max = 20 },
		size = { min = 10, max = 16 },
		sizeOverLife = { { t = 0, value = 0.6 }, { t = 1, value = 1.4 } },
		colorOverLife = fade(0.82, 0.9, 0.7, 0.35),
	}),
}

-- How far `speed` is between `range.from` and `range.to`, from 0 to 1.
local function ramp(speed: number, range: { from: number, to: number }): number
	return math.clamp((speed - range.from) / (range.to - range.from), 0, 1)
end

-- Update: what the server tells: kicks, bounces and goals.
engine.ecs.system("game.ReadPackets", {
	phase = "Update",
	side = "client",
	query = {},
	events = { read = { "net.PacketReceived" }, send = { "rtype.SpawnParticles" } },
	run = function(ctx)
		local function burst(effect: any, x: number, y: number)
			ctx.events.send("rtype.SpawnParticles", { effect = effect, position = { x = x, y = y }, layer = burstDrawLayer })
		end
		for _, packet in engine.net.received(ctx) do
			local values = packet.values
			if packet.name == "game.Kicked" then
				burst(effects.kick, values.x, values.y)
				burst(effects.kickWave, values.x, values.y)
			elseif packet.name == "game.Bounce" then
				burst(effects.bounce, values.x, values.y)
			elseif packet.name == "game.Goal" then
				State.goalScored(values.team, values.scorer, values.ownGoal)
				burst(effects.confetti[values.team] or effects.stars, values.x, values.y)
				burst(effects.stars, values.x, values.y)
			end
		end
	end,
})

-- Update: the trail on the ball and the dust under each player, once they are drawn.
engine.ecs.system("game.AddEmitters", {
	phase = "Update",
	side = "client",
	query = { with = { "rtype.SpriteRenderer" }, optionalRead = { "game.Ball", "game.Player" }, without = { "rtype.ParticleEmitter" } },
	run = function(ctx)
		for entity, ball, player in ctx.query do
			if ball == nil and player == nil then
				continue
			end
			local effect = if ball ~= nil then effects.trail else effects.dust
			local layer = if ball ~= nil then trailDrawLayer else dustDrawLayer
			ctx.commands:add(entity, "rtype.ParticleEmitter", { effect = effect, layer = layer, rateScale = 0 })
		end
	end,
})

-- Update: the faster a body goes, the more it leaves behind.
engine.ecs.system("game.UpdateEmitters", {
	phase = "Update",
	side = "client",
	query = { write = { "rtype.ParticleEmitter" }, optionalRead = { "game.Ball", "game.Player" } },
	run = function(ctx)
		for _, emitter, ball, player in ctx.query do
			-- The one-shot bursts have emitters too, and neither component: they are left alone.
			local scale
			if ball ~= nil then
				scale = ramp(Game.length(ball.velocity), trailSpeed)
			elseif player ~= nil then
				scale = ramp(Game.length(player.velocity), dustSpeed)
			else
				continue
			end
			if emitter.rateScale ~= scale then
				emitter.rateScale = scale
			end
		end
	end,
})
