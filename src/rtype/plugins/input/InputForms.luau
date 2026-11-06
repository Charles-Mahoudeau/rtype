local forms = engine.forms
local form = "engine.input.action"
local bindingKeys = { "control", "scale", "negative", "positive", "up", "down", "left", "right", "x", "y" }

-- The shapes a binding table can have, and the call each makes.
local shapes = {
	{ call = "rtype.input.bind", controls = { "control" }, numbers = { "scale" } },
	{ call = "rtype.input.bindAxis", controls = { "negative", "positive" }, numbers = {} },
	{ call = "rtype.input.bindVector", controls = { "up", "down", "left", "right" }, numbers = {} },
	{ call = "rtype.input.bindVector", controls = { "x", "y" }, numbers = {} },
}

local function hasShape(binding, shape)
	local expected = #shape.controls
	for _, key in shape.controls do
		if binding[key] == nil then
			return false
		end
	end
	for _, key in shape.numbers do
		if binding[key] ~= nil then
			expected += 1
		end
	end
	local given = 0
	for _ in binding do
		given += 1
	end
	return given == expected
end

local function readBinding(action, number, value)
	if type(value) == "string" then
		return { call = "rtype.input.bind", fields = { action = action, control = value } }
	end
	local context = form .. ": binding " .. number
	local binding = forms.table(context, value, bindingKeys)
	for _, shape in shapes do
		if hasShape(binding, shape) then
			local fields = { action = action }
			for _, key in shape.controls do
				fields[key] = forms.string(context, binding, key)
			end
			for _, key in shape.numbers do
				fields[key] = forms.number(context, binding, key)
			end
			return { call = shape.call, fields = fields }
		end
	end
	forms.fail(context, "expected a control name, {control, scale}, {negative, positive}, {up, down, left, right} or {x, y}")
	return nil
end

engine.input.action = function(...)
	forms.arguments(form, 3, ...)
	local name, kind, given = ...
	if type(name) ~= "string" or name == "" then
		forms.fail(form, "expected the action's name first")
	end
	if kind ~= nil and type(kind) ~= "string" then
		forms.fail(form, 'the type must be "button", "axis" or "vector2"')
	end
	local options = forms.options(form .. ": options", given, { "bind" })
	local planned = { { call = "rtype.input.addAction", fields = { name = name, type = kind } } }
	for index, value in forms.list(form .. ": options", options, "bind") or {} do
		table.insert(planned, readBinding(name, index, value))
	end
	if engine.hasCall("rtype.input.addAction") then
		for _, call in planned do
			forms.call(form, call.call, call.fields)
		end
	end
	-- On every side with the replication plugin, so the server and its clients agree on the PlayerInput layout.
	if engine.hasCall("rtype.replication.addInput") then
		forms.call(form, "rtype.replication.addInput", { name = name, kind = kind })
	end
end
