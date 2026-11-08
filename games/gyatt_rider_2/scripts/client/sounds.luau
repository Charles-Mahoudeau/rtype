-- The music and the sound effects. The music fades in when a match starts and loops, and fades out when it ends. A shot sounds where it was
-- fired, a hit rider where it is, and the player's own shot that hits a rider sounds for that player alone. A sound
-- whose file is missing only warns, and stays silent.

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")
local Hud = require("@scripts/client/hud")
local RiderEvents = require("@scripts/client/riderevents")
local Settings = require("@scripts/client/settings")

-- The effects, in assets/sounds/.
local shotSound = engine.assets.sound("sounds/shot.mp3") -- any rider fires
local deathSound = engine.assets.sound("sounds/death.mp3") -- a rider is hit, by a shot or the traffic
local killSound = engine.assets.sound("sounds/kill.mp3") -- the player's own shot hits a rider

local fadeSeconds = 1
local racing = false -- whether the music plays: the match races

-- Update: the music fades in when the match starts racing, then loops, and fades out when it stops racing.
engine.ecs.system("game.PlayMusic", {
	phase = "Update",
	side = "client",
	query = {},
	events = { send = { "rtype.PlayMusic", "rtype.StopMusic" } },
	after = { "game.ReadMatch" },
	run = function(ctx)
		local now = Hud.phase() == Game.racing
		if now and not racing then
			ctx.events.send("rtype.PlayMusic", { sound = assets.music, crossfade = fadeSeconds })
		elseif racing and not now then
			ctx.events.send("rtype.StopMusic", { fadeOut = fadeSeconds })
		end
		racing = now
	end,
})

-- Update: plays this frame's shots, hits and the player's kills, at the effects volume of the settings.
engine.ecs.system("game.PlayEffects", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "rtype.audio.playSound" },
	after = { "game.DetectRiderEvents" },
	run = function(ctx)
		local volume = Settings.get("effectsVolume")
		if volume <= 0 then
			return
		end
		for _, shot in RiderEvents.shots() do
			ctx.call("rtype.audio.playSound", { sound = shotSound, position = shot.from, volume = volume })
		end
		for _, hit in RiderEvents.hits() do
			ctx.call("rtype.audio.playSound", { sound = deathSound, position = hit.at, volume = volume })
		end
		if RiderEvents.kills() > 0 then
			ctx.call("rtype.audio.playSound", { sound = killSound, volume = volume })
		end
	end,
})
