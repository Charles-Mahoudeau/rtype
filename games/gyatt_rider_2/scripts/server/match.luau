-- The match: it waits for Game.minPlayers riders, races for Game.matchDuration seconds, shows its results for
-- Game.resultsDuration seconds, then waits for the next one. A player who joins during a race rides at once.

local Game = require("@scripts/shared/rules")

-- The riders of this tick, counted by game.CountRiders for game.RunMatch.
local riderCount = 0

-- The match as the other modules see it, written by game.RunMatch: its phase, its whole seconds left, and how many
-- matches started (a new one resets the scores).
local current = { phase = Game.waiting, secondsLeft = Game.matchDuration, round = 0 }

-- Startup: the match, waiting for its players.
engine.ecs.system("game.SpawnMatch", {
	phase = "Startup",
	side = "server",
	query = {},
	run = function(ctx)
		ctx.commands:spawn({
			["game.Match"] = { phase = Game.waiting, secondsLeft = Game.matchDuration },
			["game.MatchClock"] = { remaining = Game.matchDuration },
		})
	end,
})

-- FixedUpdate: counts the riders for game.RunMatch.
engine.ecs.system("game.CountRiders", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Rider" } },
	run = function(ctx)
		riderCount = 0
		for _ in ctx.query do
			riderCount += 1
		end
	end,
})

-- FixedUpdate: moves the match through its phases. The match is read, and patched only when its phase or its whole
-- seconds change: a system that goes through a component in `write` marks it changed, which sends it every tick.
engine.ecs.system("game.RunMatch", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Match" }, write = { "game.MatchClock" } },
	after = { "game.CountRiders" },
	run = function(ctx)
		for entity, match, clock in ctx.query do
			local phase = match.phase
			if phase == Game.waiting then
				if riderCount >= Game.minPlayers then
					phase, clock.remaining = Game.racing, Game.matchDuration
					current.round += 1
					print(`[server] the match starts: {Game.matchDuration} seconds`)
				end
			else
				clock.remaining = math.max(0, clock.remaining - ctx.time.fixedDelta)
				if clock.remaining <= 0 and phase == Game.racing then
					phase, clock.remaining = Game.over, Game.resultsDuration
					print("[server] the match is over")
				elseif clock.remaining <= 0 then
					phase, clock.remaining = Game.waiting, Game.matchDuration
					print(`[server] waiting for {Game.minPlayers} players`)
				end
			end
			local seconds = math.ceil(clock.remaining)
			if phase ~= match.phase or seconds ~= match.secondsLeft then
				if phase == Game.racing and phase == match.phase and seconds > 0 and seconds % 60 == 0 then
					print(`[server] {seconds // 60} minute(s) left`)
				end
				ctx.commands:patch(entity, "game.Match", { phase = phase, secondsLeft = seconds })
			end
			current.phase, current.secondsLeft = phase, seconds
		end
	end,
})

return {
	--- The match's phase: Game.waiting, Game.racing or Game.over.
	phase = function(): number
		return current.phase
	end,
	--- The whole seconds left in the match's phase.
	secondsLeft = function(): number
		return current.secondsLeft
	end,
	--- How many matches started: it changes when a new one starts.
	round = function(): number
		return current.round
	end,
	--- Whether the match races, the only time the score counts.
	racing = function(): boolean
		return current.phase == Game.racing
	end,
}
