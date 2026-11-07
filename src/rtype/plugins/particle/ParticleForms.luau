local forms = engine.forms
local form = "engine.assets.particle"
local ranges = { "lifetime", "speed", "angle", "rotation", "angularVelocity", "size" }

-- A number is both ends of a range; a table gives { min = , max = }.
local function readRange(spec, key, fields)
	local value = spec[key]
	if value == nil then
		return
	end
	if type(value) == "number" then
		fields[key .. "Min"] = value
		fields[key .. "Max"] = value
		return
	end
	local context = form .. ": '" .. key .. "'"
	if type(value) ~= "table" then
		forms.fail(context, "expected a number or a table { min = , max = }")
	end
	forms.table(context, value, { "min", "max" })
	local min = forms.number(context, value, "min")
	local max = forms.number(context, value, "max")
	if min == nil or max == nil then
		forms.fail(context, "expected a 'min' and a 'max'")
	end
	fields[key .. "Min"] = min
	fields[key .. "Max"] = max
end

-- A colour is { r, g, b, a }; the alpha may be left out and is then 1.
local function readColor(context, value)
	if type(value) ~= "table" then
		forms.fail(context, "expected a colour { r, g, b, a }")
	end
	forms.readList(context, value)
	if #value < 3 or #value > 4 then
		forms.fail(context, "expected a colour { r, g, b } or { r, g, b, a }")
	end
	for _, component in value do
		if type(component) ~= "number" then
			forms.fail(context, "a colour's components must be numbers")
		end
	end
	return { x = value[1], y = value[2], z = value[3], w = value[4] or 1 }
end

local function readShape(spec, fields)
	local value = spec.shape
	if value == nil then
		return
	end
	if type(value) == "string" then
		fields.shape = value
		return
	end
	local context = form .. ": 'shape'"
	local shape = forms.table(context, value, { "type", "radius", "edgeOnly", "size" })
	fields.shape = forms.string(context, shape, "type")
	if fields.shape == nil then
		forms.fail(context, "expected a 'type': \"point\", \"circle\" or \"rect\"")
	end
	fields.radius = forms.number(context, shape, "radius")
	fields.edgeOnly = forms.boolean(context, shape, "edgeOnly")
	fields.rectSize = forms.vec2(context, shape, "size")
end

local function readSource(spec, fields)
	if spec.source == nil then
		return
	end
	local context = form .. ": 'source'"
	local source = forms.table(context, spec.source, { "position", "size" })
	fields.sourcePosition = forms.vec2(context, source, "position")
	fields.sourceSize = forms.vec2(context, source, "size")
end

local function readFlipbook(spec, fields)
	if spec.flipbook == nil then
		return
	end
	local context = form .. ": 'flipbook'"
	local flipbook = forms.table(context, spec.flipbook, { "columns", "rows", "frames", "cycles" })
	fields.flipbookColumns = forms.count(context, flipbook, "columns")
	fields.flipbookRows = forms.count(context, flipbook, "rows")
	fields.flipbookFrames = forms.count(context, flipbook, "frames")
	fields.flipbookCycles = forms.number(context, flipbook, "cycles")
end

local function readBursts(spec)
	local bursts = {}
	for index, value in forms.list(form, spec, "bursts") or {} do
		local context = form .. ": burst " .. index
		local burst = forms.table(context, value, { "time", "count", "cycles", "interval" })
		local count = forms.count(context, burst, "count")
		if count == nil then
			forms.fail(context, "expected a 'count'")
		end
		table.insert(bursts, {
			time = forms.number(context, burst, "time"),
			count = count,
			cycles = forms.count(context, burst, "cycles"),
			interval = forms.number(context, burst, "interval"),
		})
	end
	return bursts
end

-- Each key is { t = , color = { r, g, b, a } }, or { t = , value = } for a curve; in order of t.
local function readKeys(spec, key, isColor)
	local keys = {}
	for index, value in forms.list(form, spec, key) or {} do
		local context = form .. ": '" .. key .. "' key " .. index
		local entry = forms.table(context, value, { "t", isColor and "color" or "value" })
		local t = forms.number(context, entry, "t")
		if t == nil then
			forms.fail(context, "expected a 't', from 0 at birth to 1 at death")
		end
		if isColor then
			if entry.color == nil then
				forms.fail(context, "expected a 'color'")
			end
			table.insert(keys, { t = t, color = readColor(context .. ": 'color'", entry.color) })
		else
			local number = forms.number(context, entry, "value")
			if number == nil then
				forms.fail(context, "expected a 'value'")
			end
			table.insert(keys, { t = t, value = number })
		end
	end
	return keys
end

engine.assets.particle = function(...)
	forms.arguments(form, 1, ...)
	local spec = forms.table(form, (...), {
		"name", "texture", "source", "blend", "alignment", "stretch", "flipbook", "material",
		"simulation", "capacity", "space", "sortByAge",
		"rate", "duration", "loop", "bursts", "shape",
		"lifetime", "speed", "angle", "rotation", "angularVelocity", "size", "color",
		"gravity", "drag", "colorOverLife", "sizeOverLife", "speedOverLife",
	})
	local name = forms.string(form, spec, "name")
	if name == nil or name == "" then
		forms.fail(form, "expected a 'name'")
	end
	local fields = {
		name = name,
		texture = spec.texture,
		blend = forms.string(form, spec, "blend"),
		alignment = forms.string(form, spec, "alignment"),
		stretch = forms.number(form, spec, "stretch"),
		material = spec.material,
		simulation = forms.string(form, spec, "simulation"),
		capacity = forms.count(form, spec, "capacity"),
		space = forms.string(form, spec, "space"),
		sortByAge = forms.boolean(form, spec, "sortByAge"),
		rate = forms.number(form, spec, "rate"),
		duration = forms.number(form, spec, "duration"),
		loop = forms.boolean(form, spec, "loop"),
		gravity = forms.vec2(form, spec, "gravity"),
		drag = forms.number(form, spec, "drag"),
	}
	if spec.color ~= nil then
		fields.color = readColor(form .. ": 'color'", spec.color)
	end
	for _, range in ranges do
		readRange(spec, range, fields)
	end
	readShape(spec, fields)
	readSource(spec, fields)
	readFlipbook(spec, fields)
	local bursts = readBursts(spec)
	local colorKeys = readKeys(spec, "colorOverLife", true)
	local curves = { size = readKeys(spec, "sizeOverLife", false), speed = readKeys(spec, "speedOverLife", false) }

	if not engine.hasCall("rtype.particle.addEffect") then
		return engine.assetId(name)
	end
	forms.call(form, "rtype.particle.beginEffect", fields)
	for _, burst in bursts do
		burst.name = name
		forms.call(form, "rtype.particle.addBurst", burst)
	end
	for _, key in colorKeys do
		key.name = name
		forms.call(form, "rtype.particle.addColorKey", key)
	end
	for _, curve in { "size", "speed" } do
		for _, key in curves[curve] do
			key.name = name
			key.curve = curve
			forms.call(form, "rtype.particle.addCurveKey", key)
		end
	end
	return forms.call(form, "rtype.particle.addEffect", { name = name })
end
