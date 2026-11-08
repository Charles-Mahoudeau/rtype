-- The match: it starts once each team has a player, runs Game.matchDuration seconds or until a team reaches
-- Game.scoreLimit, goes to golden goal on a draw, shows the result and starts again. A goal counts when the whole ball
-- crossed the goal line between the posts.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/server/state")

local function beginKickoff(match: any)
	match.state = Game.kickoff
	match.countdown = Game.countdown
	State.resetPositions()
end

local function startMatch(match: any)
	match.red, match.blue = 0, 0
	match.timeLeft = Game.matchDuration
	match.overtime = false
	match.winner = 0
	match.kickoffTeam = Game.red
	beginKickoff(match)
	print("[server] kickoff")
end

local function finish(match: any)
	match.state = Game.ended
	match.winner = if match.red > match.blue then Game.red else Game.blue
	match.pause = Game.endPause
	print(`[server] {Game.teamNames[match.winner]} wins {match.red}-{match.blue}`)
end

--- The team the ball is scoring for, or nil: the whole ball past a goal line, between the posts.
local function goalFor(position: { x: number, y: number }): number?
	if math.abs(position.y - Game.pitchCenter.y) > Game.goalHalfWidth then
		return nil
	end
	if position.x < Game.pitchLeft - Game.ballRadius then
		return Game.blue
	elseif position.x > Game.pitchRight + Game.ballRadius then
		return Game.red
	end
	return nil
end

-- FixedUpdate, once the ball and the players moved: the clock, the goals, and game.Match for the clients.
engine.ecs.system("game.RunMatch", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.Match" } },
	calls = { "rtype.net.sendToClients" },
	after = { "game.MirrorBall", "game.MirrorPlayers", "game.BallContacts" },
	run = function(ctx)
		local dt = ctx.time.fixedDelta
		local match = State.match()
		local full = State.teamCount(Game.red) > 0 and State.teamCount(Game.blue) > 0

		if not full then
			if match.state ~= Game.waiting then
				match.state = Game.waiting
				match.red, match.blue = 0, 0
				print("[server] waiting for a player in each team")
			end
			-- Free play: a ball in a goal goes back to the center.
			if goalFor(State.ballPosition()) ~= nil then
				beginKickoff(match)
				match.state = Game.waiting
				match.countdown = 0
			end
		elseif match.state == Game.waiting then
			startMatch(match)
		elseif match.state == Game.kickoff then
			match.countdown -= dt
			if match.countdown <= 0 then
				match.countdown = 0
				match.state = Game.playing
			end
		elseif match.state == Game.playing then
			local team = goalFor(State.ballPosition())
			if team ~= nil then
				if team == Game.red then
					match.red += 1
				else
					match.blue += 1
				end
				local touch = State.lastTouch()
				local position = State.ballPosition()
				engine.net.sendToClients(ctx, "game.Goal", {
					team = team,
					scorer = touch.name,
					ownGoal = touch.team ~= 0 and touch.team ~= team,
					x = position.x,
					y = position.y,
				})
				print(`[server] goal for {Game.teamNames[team]} by {touch.name}: {match.red}-{match.blue}`)
				State.touched("", 0)
				match.kickoffTeam = if team == Game.red then Game.blue else Game.red
				if match.overtime or match.red >= Game.scoreLimit or match.blue >= Game.scoreLimit then
					finish(match)
				else
					match.state = Game.scored
					match.pause = Game.goalPause
				end
			elseif not match.overtime then
				match.timeLeft = math.max(0, match.timeLeft - dt)
				if match.timeLeft <= 0 then
					if match.red == match.blue then
						match.overtime = true
						print("[server] golden goal")
					else
						finish(match)
					end
				end
			end
		elseif match.state == Game.scored then
			match.pause -= dt
			if match.pause <= 0 then
				beginKickoff(match)
			end
		elseif match.state == Game.ended then
			match.pause -= dt
			if match.pause <= 0 then
				startMatch(match)
			end
		end

		for _, replicated in ctx.query do
			replicated.state = match.state
			replicated.timeLeft = match.timeLeft
			replicated.countdown = match.countdown
			replicated.red = match.red
			replicated.blue = match.blue
			replicated.overtime = match.overtime
			replicated.winner = match.winner
		end
	end,
})
