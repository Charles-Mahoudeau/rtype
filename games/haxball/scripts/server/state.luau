-- What the server's systems share between them, behind functions: the ball's entity and where it is, the match's
-- state, who touched the ball last, and how many players each team has. game.Match copies the match to the clients.

local Game = require("@scripts/shared/rules")

local State = {}

local ball: any = nil
local ballPosition = { x = Game.pitchCenter.x, y = Game.pitchCenter.y }
local ballVelocity = { x = 0, y = 0 }

-- The match. `pause` counts the celebration and the end down; `resets` grows each time the bodies go back to their
-- kickoff spots, so the systems that move them notice it.
local match = {
	state = Game.waiting,
	timeLeft = Game.matchDuration,
	countdown = 0,
	red = 0,
	blue = 0,
	overtime = false,
	winner = 0,
	pause = 0,
	kickoffTeam = Game.red,
	resets = 0,
}

local lastTouch = { name = "", team = 0 }
local teamCounts = { [Game.red] = 0, [Game.blue] = 0 }

function State.setBall(entity: any)
	ball = entity
end

function State.ball(): any
	return ball
end

function State.ballMoved(position: { x: number, y: number }, velocity: { x: number, y: number })
	ballPosition = { x = position.x, y = position.y }
	ballVelocity = { x = velocity.x, y = velocity.y }
end

function State.ballPosition(): { x: number, y: number }
	return ballPosition
end

function State.ballVelocity(): { x: number, y: number }
	return ballVelocity
end

--- The match, to read and change.
function State.match(): typeof(match)
	return match
end

--- Sends every body back to its kickoff spot, on the next tick.
function State.resetPositions()
	match.resets += 1
end

--- Whether players may move: not during the countdown before a kickoff.
function State.frozen(): boolean
	return match.state == Game.kickoff
end

function State.touched(name: string, team: number)
	lastTouch = { name = name, team = team }
end

function State.lastTouch(): { name: string, team: number }
	return lastTouch
end

-- The power-ups on the pitch: { entity, kind, position }.
local powerups: { { entity: any, kind: number, position: { x: number, y: number } } } = {}

function State.powerups(): typeof(powerups)
	return powerups
end

function State.setTeamCounts(red: number, blue: number)
	teamCounts[Game.red] = red
	teamCounts[Game.blue] = blue
end

function State.teamCount(team: number): number
	return teamCounts[team] or 0
end

return State
