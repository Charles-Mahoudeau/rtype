-- The results, while a match is over: a panel in the middle with the winner, every rider's place and points in its
-- color, and whether this player won. It is spawned when the race ends and removed when the next match waits.

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")
local Hud = require("@scripts/client/hud")

local panelColor = { 0.05, 0.05, 0.07, 0.9 }
local white = { 1, 1, 1, 1 }
local winColor = { 1, 0.85, 0.3, 1 }
local loseColor = { 1, 0.45, 0.4, 1 }

-- The panel while it is shown, or nil.
local panel = nil

--- A line of text of the panel.
local function line(text: string, size: number, color)
	return { style = { ignorePointer = true }, text = { text = text, font = assets.font, size = size, color = color } }
end

--- The panel's spec, from the riders' points: the winner, the ranking, and this player's verdict.
local function resultsOf(pointsOf: { [number]: number }, ownSlot: number?)
	local ranking = {}
	for slot, points in pointsOf do
		table.insert(ranking, { slot = slot, points = points })
	end
	table.sort(ranking, function(a, b)
		return a.points > b.points
	end)
	local rows = {}
	local winner = ranking[1]
	if winner then
		table.insert(rows, line(`Player {winner.slot + 1} wins!`, 44, Hud.colorOf(winner.slot)))
	end
	for place, entry in ranking do
		table.insert(rows, line(`{place}.  Player {entry.slot + 1}   {entry.points}`, 28, Hud.colorOf(entry.slot)))
	end
	if ownSlot ~= nil and winner then
		local won = winner.slot == ownSlot
		table.insert(rows, line(if won then "You win :)" else "You lose :(", 32, if won then winColor else loseColor))
	end
	return {
		style = { width = "100%", height = "100%", justify = "center", alignItems = "center", ignorePointer = true },
		children = {
			{
				style = { padding = 32, gap = 14, alignItems = "center", ignorePointer = true },
				background = { color = panelColor },
				children = rows,
			},
		},
	}
end

-- Update: shows the results when the match turns over, and removes them when it waits again.
engine.ecs.system("game.ShowResults", {
	phase = "Update",
	side = "client",
	query = {},
	after = { "game.ReadRiders", "game.ReadMatch" },
	run = function(ctx)
		local over = Hud.phase() == Game.over
		if over and panel == nil then
			panel = engine.ui.spawn(ctx.commands, resultsOf(Hud.pointsOf(), Hud.ownSlot()))
		elseif not over and panel ~= nil then
			ctx.commands:despawn(panel)
			panel = nil
		end
	end,
})
