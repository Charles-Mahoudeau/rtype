-- The music: it fades in when the client starts, and loops for as long as it runs.

local assets = require("@scripts/client/assets")

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
