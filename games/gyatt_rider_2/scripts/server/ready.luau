-- The ready button: while the match waits, a rider's ready action (R) toggles its game.Ready, and the match starts once
-- every rider is ready (server/match.luau). When the match is not waiting, the riders are not ready any more, so the
-- next wait asks them again. game.Ready is read, and patched only when it changes.

local Game = require("@scripts/shared/rules")
local Match = require("@scripts/server/match")

-- FixedUpdate, once the inputs of the tick are stored: toggles the ready state of the riders that pressed R while the
-- match waits, and clears it once the match left the wait.
engine.ecs.system("game.ToggleReady", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Ready" }, with = { "net.PlayerInput", "game.Rider" } },
	calls = { "rtype.replication.readInput" },
	after = { "net.applyInputs" },
	run = function(ctx)
		local waiting = Match.phase() == Game.waiting
		for entity, ready in ctx.query do
			if not waiting then
				if ready.value then
					ctx.commands:patch(entity, "game.Ready", { value = false })
				end
			elseif ctx.call("rtype.replication.readInput", { entity = entity, action = "ready" }).pressed then
				ctx.commands:patch(entity, "game.Ready", { value = not ready.value })
			end
		end
	end,
})
