-- The music and the sound effects. The music fades in when the client starts and loops. A shot sounds where it was
-- fired, a hit rider where it is, and the player's own shot that hits a rider sounds for that player alone. A sound
-- whose file is missing only warns, and stays silent.

local assets = require("@scripts/client/assets")
local RiderEvents = require("@scripts/client/riderevents")

-- The effects, in assets/sounds/.
local shotSound = engine.assets.sound("sounds/shot.mp3") -- any rider fires
local deathSound = engine.assets.sound("sounds/death.mp3") -- a rider is hit, by a shot or the traffic
local killSound = engine.assets.sound("sounds/kill.mp3") -- the player's own shot hits a rider

-- Startup: the music fades in, then loops.
engine.ecs.system("game.PlayMusic", {
	phase = "Startup",
	side = "client",
	query = {},
	events = { send = { "rtype.PlayMusic" } },
	run = function(ctx)
		ctx.events.send("rtype.PlayMusic", { sound = assets.music, crossfade = 1 })
	end,
})

-- Update: plays this frame's shots, hits and the player's kills.
engine.ecs.system("game.PlayEffects", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "rtype.audio.playSound" },
	after = { "game.DetectRiderEvents" },
	run = function(ctx)
		for _, shot in RiderEvents.shots() do
			ctx.call("rtype.audio.playSound", { sound = shotSound, position = shot.from })
		end
		for _, hit in RiderEvents.hits() do
			ctx.call("rtype.audio.playSound", { sound = deathSound, position = hit.at })
		end
		if RiderEvents.kills() > 0 then
			ctx.call("rtype.audio.playSound", { sound = killSound })
		end
	end,
})
