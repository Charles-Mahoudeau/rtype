-- The music, and a sound at each shot: the laser's, or the misfire's, a little different each time.

-- The rest draws and plays: the server has none of the plugins it names.
if not engine.isClient then
	return
end

local shotPitchMin, shotPitchMax = 0.9, 1.1
local shotVolumeMin, shotVolumeMax = 0.8, 1.0

--- Plays @p sound at @p position, its pitch and volume drawn at random.
local function playShot(ctx, sound, position)
	ctx.call("rtype.audio.playSound", {
		sound = sound,
		volume = math.lerp(shotVolumeMin, shotVolumeMax, math.random()),
		pitch = math.lerp(shotPitchMin, shotPitchMax, math.random()),
		position = position,
	})
end

-- Startup: the music fades in.
engine.ecs.system("game.PlayMusic", {
	phase = "Startup",
	side = "client",
	query = {},
	events = { send = { "rtype.PlayMusic" } },
	run = function(ctx)
		ctx.events.send("rtype.PlayMusic", { sound = Duel.assets.music, crossfade = 1 })
	end,
})

-- Update: each laser the server spawned sounds where it was fired.
engine.ecs.system("game.PlayLasers", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Laser" }, added = { "game.Laser" } },
	calls = { "rtype.audio.playSound" },
	run = function(ctx)
		for _, laser in ctx.query do
			playShot(ctx, Duel.assets.laserSound, laser.position)
		end
	end,
})

-- Update: so does each misfire.
engine.ecs.system("game.PlayMisfires", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Misfire" }, added = { "game.Misfire" } },
	after = { "game.PlayLasers" },
	calls = { "rtype.audio.playSound" },
	run = function(ctx)
		for _, misfire in ctx.query do
			playShot(ctx, Duel.assets.misfireSound, misfire.position)
		end
	end,
})
