-- The music and the sound effects. The music fades in when the client starts and loops. A shot sounds where it was
-- fired, a hit rider where it is, and the player's own shot that hits a rider sounds for that player alone. A sound
-- whose file is missing only warns, and stays silent.

local assets = require("@scripts/client/assets")

-- The effects, in assets/sounds/.
local shotSound = engine.assets.sound("sounds/shot.mp3") -- any rider fires
local deathSound = engine.assets.sound("sounds/death.mp3") -- a rider is hit, by a shot or the traffic
local killSound = engine.assets.sound("sounds/kill.mp3") -- the player's own shot hits a rider

-- What each rider was at the last frame, by its entity's bits: its shots fired, whether it was a ghost, its kills.
local seenOf: { [number]: { shots: number, ghost: boolean, kills: number } } = {}

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

-- Update: a rider that fired, was just hit or, for the player's own, hit someone, sounds. A rider seen for the first
-- time only sets what it was.
engine.ecs.system("game.PlayEffects", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Shot", "game.Shield", "game.Score", "game.Position", "net.Owner" } },
	calls = { "rtype.audio.playSound", "rtype.replication.localClient" },
	run = function(ctx)
		local me = ctx.call("rtype.replication.localClient", {})
		for entity, shot, shield, score, position, owner in ctx.query do
			local seen = seenOf[entity.bits]
			seenOf[entity.bits] = { shots = shot.serial, ghost = shield.ghost, kills = score.kills }
			if seen == nil then
				continue
			end
			if shot.serial ~= seen.shots then
				ctx.call("rtype.audio.playSound", { sound = shotSound, position = shot.from })
			end
			if shield.ghost and not seen.ghost then
				ctx.call("rtype.audio.playSound", { sound = deathSound, position = position.value })
			end
			local mine = me.connected and owner.client == me.client
			if mine and score.kills ~= seen.kills then
				ctx.call("rtype.audio.playSound", { sound = killSound })
			end
		end
	end,
})
