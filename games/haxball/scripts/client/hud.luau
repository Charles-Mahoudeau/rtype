-- The HUD: the scoreboard at the top (the red score, the clock, the blue score, a status line under them) and the
-- banner in the middle (the countdown, GO, GOAL, golden goal, the winner). One system writes every node the HUD and
-- the menu change, from the state the other systems keep.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/client/state")
local assets = require("@scripts/client/assets")

local kinds = State.kinds
local white = assets.rgb(0xFFFFFF)
local dim = assets.rgb(0xB7C4BD)
local goDuration = 1 -- seconds GO stays once play starts
local overtimeBanner = 2.5 -- seconds the golden goal banner stays

local display = { flex = 0, none = 1 } -- rtype.UiNode.display

local function teamColor(team: number): { x: number, y: number, z: number, w: number }
	return assets.vec4(assets.teamColors[team] or 0xFFFFFF)
end

local function label(kind: number, size: number, color: { number }?): any
	return {
		style = { ignorePointer = true },
		text = { text = "", font = assets.font, size = size, color = color or white, align = "center" },
		components = { ["game.HudNode"] = { kind = kind } },
	}
end

local function scoreBox(team: number, kind: number): any
	return {
		style = { width = 64, paddingY = 4, alignItems = "center", ignorePointer = true },
		background = { color = assets.rgb(assets.teamColors[team]), radius = 6 },
		children = { label(kind, 28) },
	}
end

-- Startup: the scoreboard and the banner, empty until the first update.
engine.ecs.system("game.SpawnHud", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		engine.ui.spawn(ctx.commands, {
			style = { position = "absolute", top = 12, left = 0, right = 0, alignItems = "center", gap = 6, ignorePointer = true },
			children = {
				{
					style = { direction = "row", alignItems = "center", gap = 8, ignorePointer = true },
					children = {
						scoreBox(Game.red, kinds.redScore),
						{
							style = { width = 132, paddingY = 6, alignItems = "center", ignorePointer = true },
							background = { color = { 0, 0, 0, 0.55 }, radius = 6 },
							children = { label(kinds.clock, 24) },
						},
						scoreBox(Game.blue, kinds.blueScore),
					},
				},
				label(kinds.status, 13, dim),
			},
		})
		engine.ui.spawn(ctx.commands, {
			-- In the top third of the pitch: the ball and the players stay in sight at a kickoff.
			style = { width = "100%", height = "100%", paddingTop = 168, alignItems = "center", ignorePointer = true },
			children = {
				{
					style = { display = "none", paddingX = 48, paddingY = 18, alignItems = "center", gap = 6, ignorePointer = true },
					background = { color = { 0, 0, 0, 0.5 }, radius = 14 },
					components = { ["game.HudNode"] = { kind = kinds.banner } },
					children = { label(kinds.bannerTitle, 60), label(kinds.bannerSubtitle, 20) },
				},
			},
		})
	end,
})

local function clockText(match: any): string
	if match.overtime then
		return "GOLDEN"
	end
	local seconds = math.max(0, math.ceil(match.timeLeft))
	return string.format("%d:%02d", seconds // 60, seconds % 60)
end

local function statusText(match: any, me: any): string
	if not me.connected then
		return "Connecting..."
	elseif match.state == Game.waiting then
		return "Warm-up: the match starts with a player in each team"
	elseif me.team == Game.spectators then
		return "Spectating - Esc to join a team"
	elseif match.overtime then
		return "Golden goal: the next goal wins"
	end
	return ""
end

--- What the banner says now: a title, a subtitle and the title's color, or nil to hide it.
local function bannerText(match: any): (string?, string, { x: number, y: number, z: number, w: number })
	local plain = { x = 1, y = 1, z = 1, w = 1 }
	if match.state == Game.kickoff then
		return tostring(math.max(1, math.ceil(match.countdown))), "Get ready", plain
	elseif match.state == Game.playing then
		if match.overtime and match.since < overtimeBanner then
			return "GOLDEN GOAL", "The next goal wins", assets.vec4(assets.gold)
		elseif match.since < goDuration then
			return "GO!", "", plain
		end
	elseif match.state == Game.scored then
		local goal = State.lastGoal()
		local subtitle = if goal.ownGoal then `Own goal by {goal.scorer}` else `{goal.scorer} scores`
		if goal.scorer == "" then
			subtitle = ""
		end
		return "GOAL!", subtitle, teamColor(goal.team)
	elseif match.state == Game.ended then
		local winner = Game.teamNames[match.winner]
		local title = if winner then `{string.upper(winner)} WINS` else "DRAW"
		return title, `{match.red} - {match.blue}   New match soon`, teamColor(match.winner)
	end
	return nil, "", plain
end

local function setText(text: any, value: string)
	if text.text ~= value then
		text.text = value
	end
end

-- Update: every node of the HUD and the menu that changes, after the systems that keep the state.
engine.ecs.system("game.UpdateHud", {
	phase = "Update",
	side = "client",
	query = { read = { "game.HudNode" }, optionalWrite = { "rtype.UiText", "rtype.UiNode" } },
	after = { "game.TrackMatch", "game.ShowPlayers", "game.RunMenu", "game.ReadPackets" },
	run = function(ctx)
		local match = State.match()
		local me = State.me()
		local menu = State.menu()
		local title, subtitle, titleColor = bannerText(match)
		for _, hud, text, node in ctx.query do
			local kind = hud.kind
			if kind == kinds.redScore then
				setText(text, tostring(match.red))
			elseif kind == kinds.blueScore then
				setText(text, tostring(match.blue))
			elseif kind == kinds.clock then
				setText(text, clockText(match))
			elseif kind == kinds.status then
				setText(text, if menu.open then "" else statusText(match, me))
			elseif kind == kinds.banner then
				local wanted = if title ~= nil and not menu.open then display.flex else display.none
				if node.display ~= wanted then
					node.display = wanted
				end
			elseif kind == kinds.bannerTitle then
				setText(text, title or "")
				text.color = titleColor
			elseif kind == kinds.bannerSubtitle then
				setText(text, subtitle)
			elseif kind == kinds.menu then
				local wanted = if menu.open then display.flex else display.none
				if node.display ~= wanted then
					node.display = wanted
				end
			elseif kind == kinds.nameField then
				menu.name = text.text
			elseif kind == kinds.redButton then
				setText(text, `RED  {State.teamCount(Game.red)}/{Game.teamSize}`)
			elseif kind == kinds.blueButton then
				setText(text, `BLUE  {State.teamCount(Game.blue)}/{Game.teamSize}`)
			elseif kind == kinds.menuStatus then
				local status = if not me.connected
					then "Connecting to the server..."
					elseif me.team == Game.spectators then "Pick a team to play"
					else `You play for {Game.teamNames[me.team]}`
				setText(text, status)
			end
		end
	end,
})
