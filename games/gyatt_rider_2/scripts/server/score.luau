-- The score's clock: while the match races, every rider wins Game.pointsPerSecond each second; a new match starts every
-- score at 0, and the end of a race tells who won. The shots (server/shots.luau) and the hits (server/hits.luau) count
-- their own points.

local Game = require("@scripts/shared/rules")
local Match = require("@scripts/server/match")

-- What the match was at the last tick, to see when a second passes, a match starts or a race ends.
local seen = { round = 0, secondsLeft = -1, phase = Game.waiting }

-- FixedUpdate, once the match moved on and this tick's points are paid: resets the scores when a match starts, gives
-- each second's points, and tells the ranking when a race ends.
engine.ecs.system("game.CountScores", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Rider" }, write = { "game.Score" } },
	after = { "game.RunMatch", "game.RewardShooters" },
	run = function(ctx)
		local round, secondsLeft, phase = Match.round(), Match.secondsLeft(), Match.phase()
		local starts = round ~= seen.round
		local ends = phase == Game.over and seen.phase == Game.racing
		-- A second of racing passed; the race's last one passes as the match turns over.
		local ticks = (phase == Game.racing and secondsLeft ~= seen.secondsLeft and not starts) or ends
		seen.round, seen.secondsLeft, seen.phase = round, secondsLeft, phase
		if not (starts or ticks or ends) then
			return
		end
		local ranking = {}
		for _, rider, score in ctx.query do
			if starts then
				score.points = 0
			elseif ticks then
				score.points += Game.pointsPerSecond
			end
			table.insert(ranking, { slot = rider.slot, points = score.points })
		end
		if ends and #ranking > 0 then
			table.sort(ranking, function(a, b)
				return a.points > b.points
			end)
			for place, entry in ranking do
				print(`[server] {place}. player {entry.slot + 1}: {entry.points}`)
			end
			print(`[server] player {ranking[1].slot + 1} wins`)
		end
	end,
})
