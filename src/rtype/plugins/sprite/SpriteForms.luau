local forms = engine.forms
local form = "engine.assets.atlas"

local function readGrid(value, fields)
	local context = form .. ": 'grid'"
	local grid = forms.table(context, value, { "frameSize", "count", "spacing", "columns", "offset" })
	local frameSize = forms.vec2(context, grid, "frameSize")
	local count = forms.count(context, grid, "count")
	if frameSize == nil or count == nil or count == 0 then
		forms.fail(context, "expected a 'frameSize' and a 'count' of 1 or more")
	end
	fields.frameSize = frameSize
	fields.count = count
	fields.spacing = forms.vec2(context, grid, "spacing")
	fields.columns = forms.count(context, grid, "columns")
	fields.offset = forms.vec2(context, grid, "offset")
end

local function readFrame(number, value)
	local context = form .. ": frame " .. number
	local frame = forms.table(context, value, { "name", "position", "size", "origin" })
	if frame.position == nil or frame.size == nil then
		forms.fail(context, "expected a 'position' and a 'size'")
	end
	return {
		name = forms.string(context, frame, "name"),
		position = forms.vec2(context, frame, "position"),
		size = forms.vec2(context, frame, "size"),
		origin = forms.vec2(context, frame, "origin"),
	}
end

local function readFrameList(context, animation, frames, fields)
	if animation.first ~= nil or animation.count ~= nil then
		forms.fail(context, "expected either 'frames' or 'first' and 'count'")
	end
	if #frames == 0 then
		forms.fail(context, "'frames' is empty")
	end
	local first = frames[1]
	if not forms.isCount(first) then
		forms.fail(context, "'frames' must list frame indices: whole numbers, 0 or more")
	end
	for index, frame in frames do
		if not forms.isCount(frame) or frame ~= first + index - 1 then
			forms.fail(context, "'frames' must list frame indices that follow each other, such as {0, 1, 2}")
		end
	end
	fields.first = first
	fields.count = #frames
end

local function readAnimation(name, value)
	local context = form .. ": animation '" .. name .. "'"
	local animation =
		forms.table(context, value, { "frames", "first", "count", "frameDuration", "direction", "plays" })
	local fields = { name = name }
	local frames = forms.list(context, animation, "frames")
	if frames ~= nil then
		readFrameList(context, animation, frames, fields)
	else
		fields.first = forms.count(context, animation, "first")
		fields.count = forms.count(context, animation, "count")
	end
	fields.frameDuration = forms.number(context, animation, "frameDuration")
	fields.direction = forms.string(context, animation, "direction")
	fields.plays = forms.count(context, animation, "plays")
	return fields
end

engine.assets.atlas = function(...)
	forms.arguments(form, 1, ...)
	local spec = forms.table(form, (...), { "path", "texture", "name", "linear", "grid", "frames", "animations" })
	local path = forms.string(form, spec, "path")
	local texture = forms.string(form, spec, "texture")
	if (path == nil) == (texture == nil) then
		forms.fail(form, "expected either a 'path' (an atlas file) or a 'texture' (an atlas made from an image)")
	end
	if path ~= nil and (spec.name ~= nil or spec.grid ~= nil or spec.frames ~= nil) then
		forms.fail(form, "'name', 'grid' and 'frames' make an atlas from a 'texture', not from a 'path'")
	end
	local create = "rtype.sprite.loadAtlas"
	local fields = { path = path }
	local id = path
	local frames = {}
	if texture ~= nil then
		local name = forms.string(form, spec, "name")
		create = "rtype.sprite.createAtlas"
		fields = { texture = texture, name = name }
		id = name or texture
		if spec.grid ~= nil then
			readGrid(spec.grid, fields)
		end
		for index, frame in forms.list(form, spec, "frames") or {} do
			table.insert(frames, readFrame(index, frame))
		end
	end
	fields.linear = forms.boolean(form, spec, "linear")
	local animations = {}
	if spec.animations ~= nil then
		for _, entry in forms.named(form .. ": 'animations'", spec.animations) do
			table.insert(animations, readAnimation(entry[1], entry[2]))
		end
	end
	if not engine.hasCall(create) then
		return engine.assetId(id)
	end
	local atlas = forms.call(form, create, fields)
	if atlas == nil then
		forms.fail(form, "'" .. create .. "' gave no atlas")
	end
	for _, frame in frames do
		frame.atlas = atlas
		forms.call(form, "rtype.sprite.addFrame", frame)
	end
	for _, animation in animations do
		animation.atlas = atlas
		forms.call(form, "rtype.sprite.addAnimation", animation)
	end
	return atlas
end
