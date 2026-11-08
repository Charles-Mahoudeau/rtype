-- The title menu, over the game: a name, a team (each button says how full it is), spectate and quit. It opens at
-- start, closes once a team is picked, and Escape opens and closes it. game.UpdateHud writes its changing texts.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/client/state")
local assets = require("@scripts/client/assets")

local kinds = State.kinds
local white = assets.rgb(0xFFFFFF)
local dim = assets.rgb(0xB7C4BD)

-- The key that opens the menu, local to this client: the server never hears of it.
engine.call("rtype.input.addAction", { name = "menu", type = "button" })
engine.call("rtype.input.bind", { action = "menu", control = "Escape" })

local function text(value: string, size: number, color: { number }?, kind: number?): any
	return {
		style = { ignorePointer = true },
		text = { text = value, font = assets.font, size = size, color = color or white, align = "center" },
		components = if kind then { ["game.HudNode"] = { kind = kind } } else nil,
	}
end

local function button(action: string, caption: string, colors: { { number } }, kind: number?): any
	return {
		style = { grow = 1, basis = 0, paddingY = 12, alignItems = "center" },
		background = { color = colors[1], radius = 8 },
		button = { action = action, normal = colors[1], hovered = colors[2], pressed = colors[3] },
		children = { text(caption, 18, white, kind) },
	}
end

-- Each button's colors: at rest, hovered, pressed.
local redColors = { assets.rgb(0xC62828), assets.rgb(0xE53935), assets.rgb(0x8E1B1B) }
local blueColors = { assets.rgb(0x1E5BC6), assets.rgb(0x2F74E8), assets.rgb(0x163F8A) }
local greyColors = { assets.rgb(0x2A2F33), assets.rgb(0x3C4247), assets.rgb(0x1C2023) }

-- Startup: the menu, open.
engine.ecs.system("game.SpawnMenu", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		engine.ui.spawn(ctx.commands, {
			style = { width = "100%", height = "100%", justify = "center", alignItems = "center", zIndex = 10 },
			background = { color = assets.rgb(0x04100A, 0.75) },
			components = { ["game.HudNode"] = { kind = kinds.menu } },
			children = {
				{
					style = { width = 480, padding = 30, gap = 14 },
					background = { color = assets.rgb(0x0C1A13, 0.96), radius = 16, borderColor = { 1, 1, 1, 0.12 }, borderWidth = 1 },
					children = {
						text("HAXBALL", 56),
						text(`{Game.teamSize} v {Game.teamSize} football - first to {Game.scoreLimit} goals`, 14, dim),
						{ style = { height = 6 } },
						{ style = { ignorePointer = true }, text = { text = "Your name", font = assets.font, size = 13, color = dim } },
						{
							style = { padding = 10 },
							background = { color = { 0, 0, 0, 0.45 }, radius = 6, borderColor = { 1, 1, 1, 0.2 }, borderWidth = 1 },
							text = { text = "", font = assets.font, size = 18 },
							input = { action = "menu.name", placeholder = "Player", maxLength = Game.nameLength },
							components = { ["game.HudNode"] = { kind = kinds.nameField } },
						},
						{
							style = { direction = "row", gap = 12 },
							children = {
								button("menu.red", "RED", redColors, kinds.redButton),
								button("menu.blue", "BLUE", blueColors, kinds.blueButton),
							},
						},
						{
							style = { direction = "row", gap = 12 },
							children = {
								button("menu.spectate", "SPECTATE", greyColors),
								button("menu.quit", "QUIT", greyColors),
							},
						},
						text("", 14, dim, kinds.menuStatus),
						text("Move: WASD or arrows    Kick: Space or X    Menu: Esc", 12, dim),
					},
				},
			},
		})
	end,
})

local teamOfAction = { ["menu.red"] = Game.red, ["menu.blue"] = Game.blue, ["menu.spectate"] = Game.spectators }

-- Update: Escape opens and closes the menu; a team button sends the choice, with the name, and closes it.
engine.ecs.system("game.RunMenu", {
	phase = "Update",
	side = "client",
	query = {},
	events = { read = { "rtype.UiClicked", "rtype.UiTextSubmitted" }, send = { "rtype.AppExit" } },
	calls = { "rtype.input.read", "rtype.net.sendToServer" },
	after = { "game.ShowPlayers" },
	run = function(ctx)
		local menu = State.menu()
		local me = State.me()
		local wasOpen = menu.open
		if ctx.call("rtype.input.read", { action = "menu" }).pressed then
			menu.open = not menu.open
		end
		-- A hidden button keeps the focus, and Space (kick) would press it: only an open menu listens.
		local function join(team: number)
			if not me.connected then
				return
			end
			local full = team ~= Game.spectators and team ~= me.team and State.teamCount(team) >= Game.teamSize
			if not full and engine.net.sendToServer(ctx, "game.Join", { team = team, name = menu.name }) then
				menu.open = false
			end
		end
		for click in ctx.events.read("rtype.UiClicked") do
			if wasOpen then
				local team = teamOfAction[click.action]
				if team ~= nil then
					join(team)
				elseif click.action == "menu.quit" then
					ctx.events.send("rtype.AppExit", { code = 0 })
				end
			end
		end
		for submit in ctx.events.read("rtype.UiTextSubmitted") do
			-- Enter in the name field renames a player already in a team.
			if wasOpen and submit.action == "menu.name" and me.team ~= Game.spectators then
				join(me.team)
			end
		end
	end,
})
