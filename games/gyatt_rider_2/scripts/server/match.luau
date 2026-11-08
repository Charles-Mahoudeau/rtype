-- The match: it waits for Game.minPlayers riders, races for Game.matchDuration seconds, shows its results for
-- Game.resultsDuration seconds, then waits for the next one. A player who joins during a race rides at once.

local Game = require("@scripts/shared/rules")

-- The riders of this tick, counted by game.CountRiders for game.RunMatch.
local riderCount = 0

--- Puts the match in @p phase, with @p seconds to go in it.
local function enter(match, clock, phase: number, seconds: number)
	match.phase = phase
	match.secondsLeft = math.ceil(seconds)
	clock.remaining = seconds
end

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

-- FixedUpdate: moves the match through its phases. The seconds left are written only when the whole second changes.
engine.ecs.system("game.RunMatch", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.Match", "game.MatchClock" } },
	after = { "game.CountRiders" },
	run = function(ctx)
		for _, match, clock in ctx.query do
			if match.phase == Game.waiting then
				if riderCount >= Game.minPlayers then
					enter(match, clock, Game.racing, Game.matchDuration)
					print(`[server] the match starts: {Game.matchDuration} seconds`)
				end
				continue
			end
			clock.remaining = math.max(0, clock.remaining - ctx.time.fixedDelta)
			local seconds = math.ceil(clock.remaining)
			if seconds ~= match.secondsLeft then
				match.secondsLeft = seconds
				if match.phase == Game.racing and seconds > 0 and seconds % 60 == 0 then
					print(`[server] {seconds // 60} minute(s) left`)
				end
			end
			if clock.remaining > 0 then
				continue
			end
			if match.phase == Game.racing then
				enter(match, clock, Game.over, Game.resultsDuration)
				print("[server] the match is over")
			else
				enter(match, clock, Game.waiting, Game.matchDuration)
				print(`[server] waiting for {Game.minPlayers} players`)
			end
		end
	end,
})
