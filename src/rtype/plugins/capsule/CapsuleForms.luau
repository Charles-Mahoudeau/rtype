-- engine.capsule: spawns, switches and reloads capsules in one call each, from a system that lists the engine call the
-- form makes in its `calls`.
--
--   local bydo = engine.capsule.spawn(ctx, "actors/bydo.actor", {
--       ["rtype.Transform"] = { position = { x = 820, y = 140 } },
--   })
--
-- The overrides change only the fields they name, over the capsule's values: each is a ctx.commands:patch of the root,
-- applied with the system's commands, after the capsule is spawned.
local forms = engine.forms

engine.capsule = {}

-- Fails unless `ctx` is the context a system's `run` received.
local function checkContext(form, ctx)
	if type(ctx) ~= "table" or type(ctx.call) ~= "function" or ctx.commands == nil then
		forms.fail(form, "expected the system's ctx first")
	end
end

-- Fails unless `capsule` is the path of a capsule file.
local function checkCapsule(form, capsule)
	if type(capsule) ~= "string" or capsule == "" then
		forms.fail(form, 'expected the capsule\'s path second ("actors/bydo.actor")')
	end
end

-- Returns the overrides as { name, fields } pairs, sorted by component name, after checking their shape.
local function readOverrides(form, overrides)
	if overrides == nil then
		return {}
	end
	local context = form .. ": overrides"
	local entries = forms.named(context, overrides)
	for _, entry in entries do
		if type(entry[2]) ~= "table" then
			forms.fail(context, "'" .. entry[1] .. "' must be a table of fields")
		end
	end
	return entries
end

-- Makes the engine call `name` through `ctx`, its errors starting with the form.
local function call(form, ctx, name, fields)
	local done, result = pcall(ctx.call, name, fields)
	if not done then
		forms.fail(form, tostring(result))
	end
	return result
end

-- Patches the root with each override, its errors starting with the form.
local function patch(form, ctx, root, overrides)
	for _, entry in overrides do
		local done, message = pcall(ctx.commands.patch, ctx.commands, root, entry[1], entry[2])
		if not done then
			forms.fail(form .. ": overrides", tostring(message))
		end
	end
end

-- Returns a form that makes `name` with a capsule's path, then patches the root it returns with the overrides.
local function spawningForm(form, name)
	return function(...)
		forms.arguments(form, 3, ...)
		local ctx, capsule, overrides = ...
		checkContext(form, ctx)
		checkCapsule(form, capsule)
		local entries = readOverrides(form, overrides)
		local root = call(form, ctx, name, { capsule = capsule })
		patch(form, ctx, root, entries)
		return root
	end
end

-- Spawns a copy of `capsule` and returns its root. Needs "rtype.capsule.spawn" in the system's `calls`.
engine.capsule.spawn = spawningForm("engine.capsule.spawn", "rtype.capsule.spawn")

-- Despawns the capsule loaded before, spawns `capsule` and returns its root. Needs "rtype.capsule.load" in `calls`.
engine.capsule.load = spawningForm("engine.capsule.load", "rtype.capsule.load")

-- Spawns the current version of the capsule again into `root`; returns false if `root` is not a capsule's root.
-- Needs "rtype.capsule.reload" in `calls`.
function engine.capsule.reload(...)
	local form = "engine.capsule.reload"
	forms.arguments(form, 2, ...)
	local ctx, root = ...
	checkContext(form, ctx)
	if root == nil then
		forms.fail(form, "expected the capsule's root second")
	end
	return call(form, ctx, "rtype.capsule.reload", { root = root })
end
