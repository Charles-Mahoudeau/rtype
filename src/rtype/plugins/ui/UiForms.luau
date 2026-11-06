-- engine.ui: writes a whole interface as one table. Nothing here is needed: the components can be spawned by hand.
--
--   engine.ui.spawn(ctx.commands, {
--       style = { direction = "row", gap = 8, padding = 16, width = "100%" },
--       background = { color = { 0.1, 0.1, 0.1, 1 }, radius = 8 },
--       children = {
--           { text = { text = "Play", font = font } },
--           { button = { action = "menu.play" }, background = {}, style = buttonStyle },
--       },
--   })
--
-- A style is a plain table, so sharing one is sharing a table: `style = { base, { width = 100 } }` merges a list of
-- styles, the last one winning. Colours are { r, g, b, a }. Lengths are a number of pixels, "50%" or "auto".
local forms = engine.forms
local form = "engine.ui.spawn"

engine.ui = {}

local names = {
	display = { flex = 0, none = 1 },
	position = { relative = 0, absolute = 1 },
	direction = { column = 0, ["column-reverse"] = 1, row = 2, ["row-reverse"] = 3 },
	wrap = { nowrap = 0, wrap = 1, ["wrap-reverse"] = 2 },
	justify = { start = 0, center = 1, ["end"] = 2, ["space-between"] = 3, ["space-around"] = 4, ["space-evenly"] = 5 },
	align = {
		auto = 0,
		start = 1,
		center = 2,
		["end"] = 3,
		stretch = 4,
		baseline = 5,
		["space-between"] = 6,
		["space-around"] = 7,
		["space-evenly"] = 8,
	},
	overflow = { visible = 0, hidden = 1, scroll = 2 },
	textAlign = { left = 0, center = 1, right = 2 },
}

local enums = {
	display = "display",
	position = "position",
	direction = "direction",
	wrap = "wrap",
	justify = "justify",
	alignItems = "align",
	alignSelf = "align",
	alignContent = "align",
	overflow = "overflow",
}
local lengths = {
	"basis",
	"width",
	"height",
	"minWidth",
	"minHeight",
	"maxWidth",
	"maxHeight",
	"left",
	"top",
	"right",
	"bottom",
	"marginLeft",
	"marginTop",
	"marginRight",
	"marginBottom",
	"paddingLeft",
	"paddingTop",
	"paddingRight",
	"paddingBottom",
}
local numbers = { "grow", "shrink", "aspectRatio", "border", "rowGap", "columnGap", "zIndex" }
-- Shorthands: one key for several properties.
local shorthands = {
	margin = { "marginLeft", "marginTop", "marginRight", "marginBottom" },
	marginX = { "marginLeft", "marginRight" },
	marginY = { "marginTop", "marginBottom" },
	padding = { "paddingLeft", "paddingTop", "paddingRight", "paddingBottom" },
	paddingX = { "paddingLeft", "paddingRight" },
	paddingY = { "paddingTop", "paddingBottom" },
	gap = { "rowGap", "columnGap" },
}

local function length(context, key, value)
	if value == "auto" then
		return { x = 0, y = 0 }
	end
	if type(value) == "number" then
		return { x = value, y = 1 }
	end
	if type(value) == "string" then
		local percent = string.match(value, "^(-?[%d%.]+)%%$")
		if percent ~= nil then
			return { x = tonumber(percent), y = 2 }
		end
	end
	forms.fail(context, "'" .. key .. "' must be a number, \"auto\" or a percentage such as \"50%\"")
	return nil
end

local function color(value)
	return { x = value[1], y = value[2], z = value[3], w = value[4] or 1 }
end

-- A list of styles is merged in order; a single style has string keys.
local function merge(style, result)
	for key, value in style do
		if type(key) == "number" then
			merge(value, result)
		else
			result[key] = value
		end
	end
	return result
end

local function readNode(context, style)
	local merged = merge(style or {}, {})
	for key, targets in shorthands do
		if merged[key] ~= nil then
			for _, target in targets do
				if merged[target] == nil then
					merged[target] = merged[key]
				end
			end
			merged[key] = nil
		end
	end
	local node = {}
	for key, kind in enums do
		if merged[key] ~= nil then
			local value = names[kind][merged[key]]
			if value == nil then
				forms.fail(context, "unknown " .. key .. " '" .. tostring(merged[key]) .. "'")
			end
			node[key] = value
			merged[key] = nil
		end
	end
	for _, key in lengths do
		if merged[key] ~= nil then
			node[key] = length(context, key, merged[key])
			merged[key] = nil
		end
	end
	for _, key in numbers do
		if merged[key] ~= nil then
			node[key] = forms.number(context, merged, key)
			merged[key] = nil
		end
	end
	node.ignorePointer = forms.boolean(context, merged, "ignorePointer")
	merged.ignorePointer = nil
	for key in merged do
		forms.fail(context, "unknown style '" .. tostring(key) .. "'")
	end
	return node
end

local function readBackground(value)
	return {
		color = color(value.color or { 1, 1, 1, 1 }),
		cornerRadius = value.radius,
		borderColor = value.borderColor and color(value.borderColor) or nil,
		borderWidth = value.borderWidth,
	}
end

local function readImage(value)
	local slice = value.slice
	if type(slice) == "number" then
		slice = { slice, slice, slice, slice }
	end
	local source = value.source
	return {
		texture = value.texture,
		source = source and color(source) or nil,
		color = value.color and color(value.color) or nil,
		slice = slice and color(slice) or nil,
		sliceScale = value.sliceScale,
	}
end

local function readText(value)
	return {
		text = value.text,
		font = value.font,
		size = value.size,
		color = value.color and color(value.color) or nil,
		align = value.align and names.textAlign[value.align] or nil,
		wrap = value.wrap,
	}
end

local function readButton(value)
	return {
		action = value.action,
		normalColor = value.normal and color(value.normal) or nil,
		hoveredColor = value.hovered and color(value.hovered) or nil,
		pressedColor = value.pressed and color(value.pressed) or nil,
		disabledColor = value.disabled and color(value.disabled) or nil,
	}
end

local function readToggle(value)
	return {
		action = value.action,
		checked = value.checked,
		markColor = value.mark and color(value.mark) or nil,
		markInset = value.inset,
	}
end

local function readSlider(value)
	return {
		action = value.action,
		min = value.min,
		max = value.max,
		value = value.value,
		step = value.step,
		trackHeight = value.trackHeight,
		handleRadius = value.handleRadius,
		trackColor = value.track and color(value.track) or nil,
		fillColor = value.fill and color(value.fill) or nil,
		handleColor = value.handle and color(value.handle) or nil,
	}
end

local function readInput(value)
	return {
		action = value.action,
		placeholder = value.placeholder,
		maxLength = value.maxLength,
		caretColor = value.caret and color(value.caret) or nil,
	}
end

local keys = {
	"style",
	"background",
	"image",
	"text",
	"button",
	"toggle",
	"slider",
	"scroll",
	"input",
	"interaction",
	"components",
	"children",
}

-- Spawns `spec` and its children under `parent`, and returns the entity. `components` adds the game's own.
function engine.ui.spawn(commands, spec, parent)
	if not engine.isClient then
		return nil
	end
	forms.table(form, spec, keys)
	local components = {}
	for name, value in spec.components or {} do
		components[name] = value
	end
	components["rtype.UiNode"] = readNode(form, spec.style)
	if spec.background ~= nil then
		components["rtype.UiBackground"] = readBackground(spec.background)
	end
	if spec.image ~= nil then
		components["rtype.UiImage"] = readImage(spec.image)
	end
	if spec.text ~= nil then
		components["rtype.UiText"] = readText(spec.text)
	end
	local reacts = spec.button ~= nil or spec.toggle ~= nil or spec.slider ~= nil or spec.input ~= nil
	if spec.button ~= nil then
		components["rtype.UiButton"] = readButton(spec.button)
	end
	if spec.toggle ~= nil then
		components["rtype.UiToggle"] = readToggle(spec.toggle)
	end
	if spec.slider ~= nil then
		components["rtype.UiSlider"] = readSlider(spec.slider)
	end
	if spec.input ~= nil then
		components["rtype.UiTextInput"] = readInput(spec.input)
		components["rtype.UiText"] = components["rtype.UiText"] or {}
	end
	if spec.scroll ~= nil then
		components["rtype.UiScroll"] = { speed = spec.scroll.speed }
		components["rtype.UiNode"].overflow = names.overflow.scroll
	end
	if spec.interaction ~= nil or reacts then
		local interaction = spec.interaction or {}
		components["rtype.UiInteraction"] = { disabled = interaction.disabled, focusable = interaction.focusable }
	end
	local entity = commands:spawn(components)
	if parent ~= nil then
		commands:setParent(entity, parent)
	end
	for _, child in spec.children or {} do
		engine.ui.spawn(commands, child, entity)
	end
	return entity
end
