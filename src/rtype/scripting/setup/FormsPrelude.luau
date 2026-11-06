engine.assets = {}
engine.input = {}

local forms = {}
engine.forms = forms

-- Fails with a message about `context`, the form and the part of it being read.
function forms.fail(context, what)
	error(context .. ": " .. what, 0)
end

local function contains(list, item)
	return table.find(list, item) ~= nil
end

local function quoted(keys)
	local names = {}
	for _, key in keys do
		table.insert(names, "'" .. key .. "'")
	end
	return table.concat(names, ", ")
end

-- Checks that `value` is a table whose keys are all in `keys`, and returns it.
function forms.table(context, value, keys)
	if type(value) ~= "table" then
		forms.fail(context, "expected a table")
	end
	for key in value do
		if type(key) ~= "string" or not contains(keys, key) then
			local given = type(key) == "string" and "'" .. key .. "'" or "a key"
			if #keys == 0 then
				forms.fail(context, "unexpected " .. given .. ": it takes no keys")
			end
			forms.fail(context, "unexpected " .. given .. ", expected " .. quoted(keys))
		end
	end
	return value
end

-- As forms.table, but `nil` reads as an empty table.
function forms.options(context, value, keys)
	if value == nil then
		return {}
	end
	return forms.table(context, value, keys)
end

-- Each of these returns the value of `key`, or nil if the table does not have it.
function forms.boolean(context, spec, key)
	local value = spec[key]
	if value ~= nil and type(value) ~= "boolean" then
		forms.fail(context, "'" .. key .. "' must be a boolean")
	end
	return value
end

function forms.number(context, spec, key)
	local value = spec[key]
	if value ~= nil and type(value) ~= "number" then
		forms.fail(context, "'" .. key .. "' must be a number")
	end
	return value
end

function forms.isCount(value)
	return type(value) == "number" and value >= 0 and value <= 4294967295 and math.floor(value) == value
end

function forms.count(context, spec, key)
	local value = spec[key]
	if value ~= nil and not forms.isCount(value) then
		forms.fail(context, "'" .. key .. "' must be a whole number, 0 or more")
	end
	return value
end

function forms.string(context, spec, key)
	local value = spec[key]
	if value ~= nil and type(value) ~= "string" then
		forms.fail(context, "'" .. key .. "' must be a string")
	end
	return value
end

function forms.vec2(context, spec, key)
	local value = spec[key]
	if value == nil then
		return nil
	end
	local part = context .. ": '" .. key .. "'"
	if type(value) ~= "table" then
		error(part .. " must be a table {x, y}", 0)
	end
	forms.table(part, value, { "x", "y" })
	for _, axis in { "x", "y" } do
		if forms.number(part, value, axis) == nil then
			forms.fail(part, "missing '" .. axis .. "'")
		end
	end
	return value
end

-- Returns the list `value`, which must have no holes and no named keys.
function forms.readList(context, value)
	if type(value) ~= "table" then
		forms.fail(context, "expected a table")
	end
	local count = 0
	for key in value do
		if not forms.isCount(key) or key == 0 then
			forms.fail(context, "expected a list, without holes or named keys")
		end
		count += 1
	end
	if count ~= #value then
		forms.fail(context, "expected a list, without holes or named keys")
	end
	return value
end

function forms.list(context, spec, key)
	local value = spec[key]
	if value == nil then
		return nil
	end
	return forms.readList(context .. ": '" .. key .. "'", value)
end

-- Returns the entries of `value` as { name, value } pairs, sorted by name.
function forms.named(context, value)
	if type(value) ~= "table" then
		forms.fail(context, "expected a table")
	end
	local entries = {}
	for key, item in value do
		if type(key) ~= "string" then
			forms.fail(context, "every key must be a name")
		end
		table.insert(entries, { key, item })
	end
	table.sort(entries, function(left, right)
		return left[1] < right[1]
	end)
	return entries
end

-- Fails if a form got more than `count` arguments.
function forms.arguments(form, count, ...)
	local given = select("#", ...)
	if given > count then
		forms.fail(form, "expected at most " .. count .. " argument(s), got " .. given)
	end
end

-- Makes an engine call, its errors starting with the form.
function forms.call(form, name, fields)
	local results = table.pack(pcall(engine.call, name, fields))
	if not results[1] then
		forms.fail(form, (string.gsub(tostring(results[2]), "^native function 'call': ", "")))
	end
	return table.unpack(results, 2, results.n)
end

-- Returns a form that asks for one file: `form(path, options)`, whose boolean options go to `call` as they are named.
-- Without the plugin, it gives the id the asset would have.
function forms.file(form, call, options)
	return function(...)
		forms.arguments(form, 2, ...)
		local path, given = ...
		if type(path) ~= "string" or path == "" then
			forms.fail(form, "expected the file's path first")
		end
		local context = form .. ": options"
		local spec = forms.options(context, given, options)
		local fields = { path = path }
		for _, option in options do
			fields[option] = forms.boolean(context, spec, option)
		end
		if not engine.hasCall(call) then
			return engine.assetId(path)
		end
		return forms.call(form, call, fields)
	end
end
