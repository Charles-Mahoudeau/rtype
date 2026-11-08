-- The HUD, drawn with the UI over the game: the four scores along the top, each on a text box (client/textbox.luau)
-- in its player's color, the time left in the strip on the left, the player's own score on a text box in the bottom
-- right corner, and a message on a text box in the middle while the match waits. Its texts carry a game.HudText, and
-- game.ShowHud rewrites them from what the server sends. It also tells the other modules what it read (the results,
-- client/results.luau).

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")
local TextBox = require("@scripts/client/textbox")
local Menu = require("@scripts/client/menu")

-- What a game.HudText shows.
local scoreText, timerText, ownScoreText, messageText, readyText = 0, 1, 2, 3, 4

-- The players' colors, by slot (the riders' art), and the look of the boxes.
local playerColors = {
	{ 0.43, 0.76, 1, 1 }, -- blue
	{ 1, 0.6, 0.6, 1 }, -- red
	{ 0.49, 0.99, 0.49, 1 }, -- green
	{ 0.84, 0.61, 1, 1 }, -- purple
}
local ink = TextBox.ink
local white = { 1, 1, 1, 1 }
local readyColor = { 0.49, 0.99, 0.49, 1 }
local timerColor = { 1, 0.55, 0, 1 }
local ownColor = { 0.8, 0.82, 0.85, 1 }
local scoreBox = { width = 220, height = 52, ignorePointer = true }
local margin = 8

-- The waiting message's box while it is shown, or nil.
local waitingBox = nil

-- What game.ReadRiders and game.ReadMatch saw this frame, for game.ShowHud.
local pointsOf: { [number]: number } = {} -- by slot
local ownSlot: number? = nil
local riders, readyCount = 0, 0
local ownReady = false
local match = { phase = Game.waiting, secondsLeft = Game.matchDuration }

--- A text node of the HUD: its `role` and `slot` say what game.ShowHud writes in it.
local function textNode(role: number, slot: number, size: number, color)
	return {
		style = { ignorePointer = true },
		text = { text = "", font = assets.font, size = size, color = color, align = "center" },
		components = { ["game.HudText"] = { role = role, slot = slot } },
	}
end

--- A plain box of color @p color holding one small text node.
local function box(style, color, text)
	return {
		style = { style, { justify = "center", alignItems = "center", ignorePointer = true } },
		background = { color = color },
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
			table.insert(scores, TextBox.node(scoreBox, playerColors[slot + 1], { textNode(scoreText, slot, 26, ink) }))
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
				TextBox.node(
					{ position = "absolute", right = 16, bottom = 16, width = 240, height = 80, ignorePointer = true },
					ownColor,
					{ textNode(ownScoreText, 0, 36, ink) }
				),
			},
		})
	end,
})

-- Update: the riders' points by slot, how many ride, and the slot of this client's own rider.
engine.ecs.system("game.ReadRiders", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Rider", "game.Score", "game.Ready", "net.Owner" } },
	calls = { "rtype.replication.localClient" },
	run = function(ctx)
		local me = ctx.call("rtype.replication.localClient", {})
		table.clear(pointsOf)
		ownSlot, riders, readyCount, ownReady = nil, 0, 0, false
		for _, rider, score, ready, owner in ctx.query do
			pointsOf[rider.slot] = score.points
			riders += 1
			if ready.value then
				readyCount += 1
			end
			if me.connected and owner.client == me.client then
				ownSlot = rider.slot
				ownReady = ready.value
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
	elseif hud.role == readyText then
		if ownSlot == nil then
			return "Watching"
		end
		return if ownReady then "Ready!  (R to cancel)" else "Press R when ready"
	elseif match.phase == Game.waiting then
		if riders < Game.minPlayers then
			return `Waiting for players ({riders}/{Game.minPlayers})`
		end
		return `Everyone ready? ({readyCount}/{riders})`
	end
	return ""
end

-- Update: the waiting message's text box, shown while the match waits for riders and the menu is closed (its text
-- would show through the menu); game.ShowHud writes its text.
engine.ecs.system("game.ShowWaiting", {
	phase = "Update",
	side = "client",
	query = {},
	after = { "game.ReadMatch" },
	run = function(ctx)
		local waiting = match.phase == Game.waiting and not Menu.isOpen()
		if waiting and waitingBox == nil then
			waitingBox = engine.ui.spawn(ctx.commands, {
				style = { width = "100%", height = "100%", justify = "center", alignItems = "center", ignorePointer = true },
				children = {
					TextBox.node(
						{ width = 760, height = 120, ignorePointer = true },
						white,
						{ textNode(messageText, 0, 36, ink) }
					),
					TextBox.node(
						{ width = 520, height = 90, marginTop = 16, ignorePointer = true },
						white,
						{ textNode(readyText, 0, 28, ink) },
						{ components = { ["game.ReadyButton"] = {} } }
					),
				},
			})
		elseif not waiting and waitingBox ~= nil then
			ctx.commands:despawn(waitingBox)
			waitingBox = nil
		end
	end,
})

-- Update: the ready button is green once the player is ready, white until then.
engine.ecs.system("game.ColorReadyButton", {
	phase = "Update",
	side = "client",
	query = { write = { "rtype.UiImage" }, with = { "game.ReadyButton" } },
	after = { "game.ReadRiders" },
	before = { "game.LightMenuButtons" },
	run = function(ctx)
		local color = if ownReady then readyColor else white
		for _, image in ctx.query do
			if image.color.x ~= color[1] then
				image.color = { x = color[1], y = color[2], z = color[3], w = color[4] }
			end
		end
	end,
})

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

return {
	--- The riders' points by slot, as read this frame.
	pointsOf = function(): { [number]: number }
		return table.clone(pointsOf)
	end,
	--- The slot of this client's own rider, or nil while it has none.
	ownSlot = function(): number?
		return ownSlot
	end,
	--- The match's phase: Game.waiting, Game.racing or Game.over.
	phase = function(): number
		return match.phase
	end,
	--- A player's color by slot, as the HUD draws it: { r, g, b, a }.
	colorOf = function(slot: number): { number }
		return table.clone(playerColors[slot + 1])
	end,
}
