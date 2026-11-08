local forms = engine.forms
engine.persistence = {}

-- Makes `call` through the system's ctx (which lists it under `calls`), or through the engine while the script loads.
local function invoke(form, ctx, call, fields)
	if ctx ~= nil and type(ctx) ~= "table" then
		forms.fail(form, "expected a system's ctx (or nil) first")
	end
	if ctx == nil then
		return forms.call(form, call, fields)
	end
	local results = table.pack(pcall(ctx.call, call, fields))
	if not results[1] then
		forms.fail(form, tostring(results[2]))
	end
	return results[2]
end

local function nameOf(form, given, fields)
	local spec = forms.table(form, given, fields or { "name" })
	local name = forms.string(form, spec, "name")
	if name == nil or name == "" then
		forms.fail(form, "expected a 'name'")
	end
	return name
end

-- engine.persistence.read(ctx, { name }) -> { ok, data, error }. A missing or corrupt save is `ok = false` with
-- error "not_found" or "corrupt", never a script error.
engine.persistence.read = function(...)
	forms.arguments("engine.persistence.read", 2, ...)
	local ctx, given = ...
	return invoke("engine.persistence.read", ctx, "rtype.persistence.read", { name = nameOf("engine.persistence.read", given) })
end

-- engine.persistence.write(ctx, { name, data }) -> { ok, error }. Atomic: a failure leaves the old save intact.
engine.persistence.write = function(...)
	local form = "engine.persistence.write"
	forms.arguments(form, 2, ...)
	local ctx, given = ...
	local spec = forms.table(form, given, { "name", "data" })
	local data = forms.string(form, spec, "data")
	if data == nil then
		forms.fail(form, "expected a 'data' string")
	end
	return invoke(form, ctx, "rtype.persistence.write", { name = nameOf(form, given, { "name", "data" }), data = data })
end

-- engine.persistence.delete(ctx, { name }) -> { ok, error }. Deleting a save that does not exist succeeds.
engine.persistence.delete = function(...)
	forms.arguments("engine.persistence.delete", 2, ...)
	local ctx, given = ...
	return invoke("engine.persistence.delete", ctx, "rtype.persistence.delete", { name = nameOf("engine.persistence.delete", given) })
end

-- engine.persistence.exists(ctx, { name }) -> { exists }.
engine.persistence.exists = function(...)
	forms.arguments("engine.persistence.exists", 2, ...)
	local ctx, given = ...
	return invoke("engine.persistence.exists", ctx, "rtype.persistence.exists", { name = nameOf("engine.persistence.exists", given) })
end
