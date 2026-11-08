-- The HUD, drawn with the UI over the game: the four scores along the top, in the players' colors, the time left in the
-- strip on the left, the player's own score in the bottom right corner, and a message in the middle while the match
-- waits or is over. Its texts carry a game.HudText, and game.ShowHud rewrites them from what the server sends.

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")

-- What a game.HudText shows.
local scoreText, timerText, ownScoreText, messageText = 0, 1, 2, 3

-- The players' colors, by slot (the riders' art), and the look of the boxes.
local playerColors = {
	{ 0.43, 0.76, 1, 1 }, -- blue
	{ 1, 0.6, 0.6, 1 }, -- red
	{ 0.49, 0.99, 0.49, 1 }, -- green
	{ 0.84, 0.61, 1, 1 }, -- purple
}
local ink = { 0.08, 0.08, 0.1, 1 }
local white = { 1, 1, 1, 1 }
local timerColor = { 1, 0.55, 0, 1 }
local ownColor = { 0.8, 0.82, 0.85, 1 }
local scoreBox = { width = 220, height = 44 }
local margin = 8

-- What game.ReadRiders and game.ReadMatch saw this frame, for game.ShowHud.
local pointsOf: { [number]: number } = {} -- by slot
local ownSlot: number? = nil
local riders = 0
local match = { phase = Game.waiting, secondsLeft = Game.matchDuration }

--- A text node of the HUD: its `role` and `slot` say what game.ShowHud writes in it.
local function textNode(role: number, slot: number, size: number, color)
	return {
		style = { ignorePointer = true },
		text = { text = "", font = assets.font, size = size, color = color, align = "center" },
		components = { ["game.HudText"] = { role = role, slot = slot } },
	}
end

--- A box of color @p color holding one text node.
local function box(style, color, text)
	return {
		style = { style, { justify = "center", alignItems = "center", ignorePointer = true } },
		background = { color = color, radius = 6 },
		children = { text },
	}
end

-- Startup: spawns the HUD's nodes, every text empty until game.ShowHud fills it.
engine.ecs.system("game.SpawnHud", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		local scores = {}
		for slot = 0, Game.maxPlayers - 1 do
			table.insert(scores, box(scoreBox, playerColors[slot + 1], textNode(scoreText, slot, 24, ink)))
		end
		engine.ui.spawn(ctx.commands, {
			style = { width = "100%", height = "100%", ignorePointer = true },
			children = {
				{
					style = {
						position = "absolute",
						top = margin,
						left = Game.roadsLeft + margin,
						right = margin,
						direction = "row",
						justify = "space-evenly",
						ignorePointer = true,
					},
					children = scores,
				},
				box(
					{ position = "absolute", top = margin, left = 4, width = Game.roadsLeft - 8, height = 44 },
					timerColor,
					textNode(timerText, 0, 18, ink)
				),
				box(
					{ position = "absolute", right = 16, bottom = 16, width = 220, height = 72 },
					ownColor,
					textNode(ownScoreText, 0, 36, ink)
				),
				{
					style = { position = "absolute", top = "45%", left = 0, right = 0, alignItems = "center" },
					children = { textNode(messageText, 0, 40, white) },
				},
			},
		})
	end,
})

-- Update: the riders' points by slot, how many ride, and the slot of this client's own rider.
engine.ecs.system("game.ReadRiders", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Rider", "game.Score", "net.Owner" } },
	calls = { "rtype.replication.localClient" },
	run = function(ctx)
		local me = ctx.call("rtype.replication.localClient", {})
		table.clear(pointsOf)
		ownSlot, riders = nil, 0
		for _, rider, score, owner in ctx.query do
			pointsOf[rider.slot] = score.points
			riders += 1
			if me.connected and owner.client == me.client then
				ownSlot = rider.slot
			end
		end
	end,
})

-- Update: the match's phase and time left.
engine.ecs.system("game.ReadMatch", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Match" } },
	run = function(ctx)
		for _, current in ctx.query do
			match.phase, match.secondsLeft = current.phase, current.secondsLeft
		end
	end,
})

--- What a HUD text shows now.
local function textOf(hud): string
	if hud.role == scoreText then
		local points = pointsOf[hud.slot]
		return if points then tostring(points) else ""
	elseif hud.role == timerText then
		local seconds = if match.phase == Game.racing then match.secondsLeft else Game.matchDuration
		return string.format("%d:%02d", seconds // 60, seconds % 60)
	elseif hud.role == ownScoreText then
		return if ownSlot then tostring(pointsOf[ownSlot] or 0) else ""
	elseif match.phase == Game.waiting then
		return `Waiting for players ({riders}/{Game.minPlayers})`
	elseif match.phase == Game.over then
		return "Match over"
	end
	return ""
end

-- Update: writes each HUD text, only when it changes. The box of a slot nobody rides stays empty.
engine.ecs.system("game.ShowHud", {
	phase = "Update",
	side = "client",
	query = { read = { "game.HudText" }, write = { "rtype.UiText" } },
	after = { "game.ReadRiders", "game.ReadMatch" },
	run = function(ctx)
		for _, hud, text in ctx.query do
			local wanted = textOf(hud)
			if text.text ~= wanted then
				text.text = wanted
			end
		end
	end,
})
