-- Every widget of the UI. Styles are plain tables: sharing one is sharing a table.
local font = engine.assets.font("fonts/orbitron.json")
local panel = engine.assets.texture("panel.png")

local white = { 1, 1, 1, 1 }
local blue = { 0.25, 0.45, 0.85, 1 }

local title = { font = font, size = 34, color = white }
local label = { font = font, size = 18, color = white }
local row = { direction = "row", alignItems = "center", gap = 12 }

local function button(name, action)
	return {
		style = { padding = 12, alignItems = "center" },
		background = { color = blue, radius = 8 },
		button = { action = action, normal = blue, hovered = { 0.35, 0.6, 1, 1 }, pressed = { 0.15, 0.3, 0.6, 1 } },
		children = { { style = { ignorePointer = true }, text = { text = name, font = font, size = 20 } } },
	}
end

local function listItem(index)
	return {
		style = { padding = 8, marginBottom = 4 },
		background = { color = { 1, 1, 1, 0.08 }, radius = 4 },
		children = { { text = { text = `Save slot {index}`, font = font, size = 16 } } },
	}
end

local items = {}
for index = 1, 20 do
	table.insert(items, listItem(index))
end

engine.ecs.system("demo.SpawnMenu", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		engine.ui.spawn(ctx.commands, {
			style = { width = "100%", height = "100%", direction = "row", padding = 24, gap = 24 },
			background = { color = { 0.06, 0.07, 0.1, 1 } },
			children = {
				{
					style = { grow = 1, shrink = 1, basis = 0, padding = 24, gap = 16 },
					image = { texture = panel, slice = 8 },
					children = {
						{ text = { text = "Options", font = font, size = title.size } },
						button("Play", "menu.play"),
						button("Quit", "menu.quit"),
						{
							style = row,
							children = {
								{ style = { width = 80 }, text = { text = "Volume", font = font, size = 18 } },
								{ style = { grow = 1, height = 24 }, slider = { action = "menu.volume", value = 0.6 } },
							},
						},
						{
							style = row,
							children = {
								{
									style = { width = 24, height = 24 },
									background = { color = { 0, 0, 0, 0.4 }, radius = 4, borderColor = white, borderWidth = 1 },
									toggle = { action = "menu.fullscreen", checked = true, mark = blue },
								},
								{ text = { text = "Fullscreen", font = font, size = label.size } },
							},
						},
						{
							style = { padding = 8 },
							background = { color = { 0, 0, 0, 0.4 }, radius = 4 },
							text = { text = "", font = font, size = 18 },
							input = { action = "menu.name", placeholder = "Your name", maxLength = 24 },
						},
						{
							text = {
								text = "This text is long enough to be wrapped by the layout, between its words, whatever the width of the panel it is in, which comes from Yoga.",
								font = font,
								size = 14,
								wrap = true,
							},
						},
					},
				},
				{
					style = { width = 300, overflow = "hidden" },
					background = { color = { 1, 1, 1, 0.05 }, radius = 8 },
					scroll = { speed = 40 },
					children = items,
				},
			},
		})
	end,
})

engine.ecs.system("demo.ReadMenu", {
	phase = "Update",
	side = "client",
	query = {},
	events = { read = { "rtype.UiClicked", "rtype.UiValueChanged", "rtype.UiTextSubmitted" }, send = { "rtype.AppExit" } },
	run = function(ctx)
		for click in ctx.events.read("rtype.UiClicked") do
			print(`clicked {click.action}`)
			if click.action == "menu.quit" then
				ctx.events.send("rtype.AppExit", { code = 0 })
			end
		end
		for change in ctx.events.read("rtype.UiValueChanged") do
			print(`{change.action} = {change.value}`)
		end
		for submit in ctx.events.read("rtype.UiTextSubmitted") do
			print(`submitted {submit.action}`)
		end
	end,
})
