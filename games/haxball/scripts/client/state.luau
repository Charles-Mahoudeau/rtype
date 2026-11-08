-- What the client's systems share: the match as the server sent it, the last goal, who this client is, and the menu.

local Game = require("@scripts/shared/rules")

local State = {}

-- The nodes of the HUD and the menu a system changes, as game.HudNode.kind says.
State.kinds = table.freeze({
	redScore = 1,
	blueScore = 2,
	clock = 3,
	status = 4, -- the line under the scoreboard
	banner = 5, -- the panel in the middle, hidden when it has nothing to say
	bannerTitle = 6,
	bannerSubtitle = 7,
	menu = 10,
	nameField = 11,
	redButton = 12,
	blueButton = 13,
	menuStatus = 14,
})

local match = {
	state = Game.waiting,
	timeLeft = Game.matchDuration,
	countdown = 0,
	red = 0,
	blue = 0,
	overtime = false,
	winner = 0,
	-- Seconds since the state last changed, counted by the client: the banners use it.
	since = 0,
}
local lastGoal = { team = 0, scorer = "", ownGoal = false }
local me = { connected = false, client = 0, team = Game.spectators }
local teamCounts = { [Game.red] = 0, [Game.blue] = 0 }
local menu = { open = true, name = "" }

--- The match, as the last game.Match said.
function State.match(): typeof(match)
	return match
end

function State.goalScored(team: number, scorer: string, ownGoal: boolean)
	lastGoal = { team = team, scorer = scorer, ownGoal = ownGoal }
end

function State.lastGoal(): typeof(lastGoal)
	return lastGoal
end

--- This client: whether the server gave it its id, the id, and its team.
function State.me(): typeof(me)
	return me
end

function State.setTeamCounts(red: number, blue: number)
	teamCounts[Game.red] = red
	teamCounts[Game.blue] = blue
end

function State.teamCount(team: number): number
	return teamCounts[team] or 0
end

--- The menu: whether it shows, and the name typed in it.
function State.menu(): typeof(menu)
	return menu
end

return State
