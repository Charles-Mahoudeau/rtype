-- The menu, opened and closed with Escape, over the game, which goes on: the player's settings (client/settings.luau),
-- how to play, and quitting. Every line sits on a text box (client/textbox.luau); the focused or hovered button turns
-- orange. The keyboard (Tab, the arrows, Enter), the mouse and a gamepad all work it.

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")
local Settings = require("@scripts/client/settings")
local TextBox = require("@scripts/client/textbox")

local white = { 1, 1, 1, 1 }
local orange = { 1, 0.6, 0.2, 1 }
local shade = { 0, 0, 0, 0.55 }
local wide = 640 -- pixels, the width of the menu's lines

engine.call("rtype.input.addAction", { name = "menu", type = "button" })
engine.call("rtype.input.bind", { action = "menu", control = "Escape" })

-- The menu's root while it is open, or nil.
local root = nil

--- A button on a text box: @p label, sending @p action when activated.
local function button(label: string, action: string)
	return TextBox.node({ width = 440, height = 76 }, white, { TextBox.text(label, 28) }, {
		button = { action = action },
		components = { ["game.MenuButton"] = {} },
	})
end

--- A line of a setting on a text box: @p label, then @p widget.
local function setting(label: string, widget)
	return TextBox.node({ { width = wide, height = 84, direction = "row", gap = 28 } }, white, {
		TextBox.text(label, 24),
		widget,
	})
end

--- A box to tick, sending @p action with 1 or 0.
local function toggle(action: string, checked: boolean)
	return {
		style = { width = 30, height = 30 },
		background = { color = { 0, 0, 0, 0.2 }, borderColor = TextBox.ink, borderWidth = 2 },
		toggle = { action = action, checked = checked, mark = orange },
	}
end

--- The whole screen, shaded, with @p children in a column in its middle, over the rest of the interface. The shade
--- is an image, the white square tinted: the UI draws the images after the backgrounds, so a background would stay
--- under the HUD's text boxes. The texts are drawn after every image, so the HUD hides its middle message while the
--- menu is open (client/hud.luau).
local function page(children)
	return {
		style = { width = "100%", height = "100%", justify = "center", alignItems = "center", gap = 14, zIndex = 10 },
		image = { texture = assets.white, color = shade },
		children = children,
	}
end

local function mainPage()
	return page({
		TextBox.node({ width = wide, height = 120 }, orange, { TextBox.text("GyattRider 2", 48) }),
		button("Resume", "menu.resume"),
		setting("Effects volume", {
			style = { width = 220, height = 26 },
			slider = { action = "menu.volume", value = Settings.get("effectsVolume"), track = { 0, 0, 0, 0.25 } },
		}),
		setting("Reduce motion", toggle("menu.motion", Settings.get("reduceMotion"))),
		setting("Boost icons", toggle("menu.icons", Settings.get("boostIcons"))),
		button("How to play", "menu.help"),
		button("Quit", "menu.quit"),
	})
end

--- A line of the help on a text box.
local function help(text: string)
	return TextBox.node({ width = 900, height = 64 }, white, { TextBox.text(text, 22) })
end

local function helpPage()
	return page({
		TextBox.node({ width = wide, height = 110 }, orange, { TextBox.text("How to play", 44) }),
		help("ZQSD (WASD): ride      Arrows: fire a beam"),
		help(`A shot costs {Game.shotCost}; hitting a rider or a cyclist gives {Game.hitReward}`),
		help("A hit rider loses a tenth of its points to its shooter"),
		help("Shoot a colored cyclist for its boost: gold points,"),
		help("cyan speed, green invincible, red sniper"),
		help(`The best score after {Game.matchDuration // 60} minutes wins`),
		button("Back", "menu.back"),
	})
end

--- Shows @p spec as the menu, in place of what it showed; nil closes it.
local function show(ctx, spec)
	if root ~= nil then
		ctx.commands:despawn(root)
		root = nil
	end
	if spec ~= nil then
		root = engine.ui.spawn(ctx.commands, spec)
	end
end

-- Update: Escape opens or closes the menu; its buttons, slider and boxes change the settings, show the help, or quit.
engine.ecs.system("game.RunMenu", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "rtype.input.read" },
	events = { read = { "rtype.UiClicked", "rtype.UiValueChanged" }, send = { "rtype.AppExit" } },
	before = { "game.Quit" },
	run = function(ctx)
		if ctx.call("rtype.input.read", { action = "menu" }).pressed then
			show(ctx, if root == nil then mainPage() else nil)
		end
		for click in ctx.events.read("rtype.UiClicked") do
			if click.action == "menu.resume" then
				show(ctx, nil)
			elseif click.action == "menu.help" then
				show(ctx, helpPage())
			elseif click.action == "menu.back" then
				show(ctx, mainPage())
			elseif click.action == "menu.quit" then
				ctx.events.send("rtype.AppExit", { code = 0 })
			end
		end
		for change in ctx.events.read("rtype.UiValueChanged") do
			if change.action == "menu.volume" then
				Settings.set("effectsVolume", change.value)
			elseif change.action == "menu.motion" then
				Settings.set("reduceMotion", change.value ~= 0)
			elseif change.action == "menu.icons" then
				Settings.set("boostIcons", change.value ~= 0)
			end
		end
	end,
})

-- Update: the menu's button under the pointer or the keyboard's focus is orange, the others white.
engine.ecs.system("game.LightMenuButtons", {
	phase = "Update",
	side = "client",
	query = { read = { "rtype.UiInteraction" }, write = { "rtype.UiImage" }, with = { "game.MenuButton" } },
	run = function(ctx)
		for _, interaction, image in ctx.query do
			local lit = interaction.hovered or interaction.focused
			local color = if lit then orange else white
			if image.color.y ~= color[2] then
				image.color = { x = color[1], y = color[2], z = color[3], w = color[4] }
			end
		end
	end,
})

return {
	--- Whether the menu is open.
	isOpen = function(): boolean
		return root ~= nil
	end,
}
