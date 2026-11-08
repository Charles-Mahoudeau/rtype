-- The helpers of the debug console (ScriptHost::evaluate and complete). Run once; returns
-- { format = ..., complete = ..., engineView = ... }.

local MAX_DEPTH = 3 -- how many tables deep a result is written
local MAX_ITEMS = 24 -- how many entries of one table are written
local MAX_CANDIDATES = 64 -- how many completions are given

local KEYWORDS = {
	"and", "break", "continue", "do", "else", "elseif", "end", "false", "for", "function", "if", "in", "local",
	"nil", "not", "or", "repeat", "return", "then", "true", "until", "while",
}

local function isIdentifier(text)
	return string.match(text, "^[%a_][%w_]*$") ~= nil
end

-- Keys in a stable order: numbers first, then strings, then the rest by their text.
local function sortedKeys(source)
	local keys = {}
	for key in source do
		table.insert(keys, key)
	end
	table.sort(keys, function(a, b)
		local ta, tb = type(a), type(b)
		if ta ~= tb then
			return ta == "number"
		end
		if ta == "number" or ta == "string" then
			return a < b
		end
		return tostring(a) < tostring(b)
	end)
	return keys
end

local show

local function showKey(key)
	if type(key) == "string" and isIdentifier(key) then
		return key
	end
	return "[" .. show(key, MAX_DEPTH, {}) .. "]"
end

function show(value, depth, seen)
	local kind = typeof(value)
	if kind == "string" then
		return string.format("%q", value)
	end
	if kind ~= "table" then
		return tostring(value)
	end
	if seen[value] then
		return "{...}"
	end
	local keys = sortedKeys(value)
	if #keys == 0 then
		return "{}"
	end
	if depth >= MAX_DEPTH then
		return "{... " .. #keys .. " entries}"
	end
	seen[value] = true
	local parts = {}
	for index, key in keys do
		if index > MAX_ITEMS then
			table.insert(parts, "... " .. (#keys - MAX_ITEMS) .. " more")
			break
		end
		local inner = show(value[key], depth + 1, seen)
		if type(key) == "number" and key == index then
			table.insert(parts, inner)
		else
			table.insert(parts, showKey(key) .. " = " .. inner)
		end
	end
	seen[value] = nil
	return "{ " .. table.concat(parts, ", ") .. " }"
end

-- Writes the values an expression gave, separated by commas. No value gives an empty text.
local function format(...)
	local count = select("#", ...)
	local parts = {}
	for index = 1, count do
		table.insert(parts, show((select(index, ...)), 0, {}))
	end
	return table.concat(parts, ", ")
end

-- The names of the entries of `value` that a script can reach with a dot.
local function namesOf(value)
	local names = {}
	if type(value) == "table" then
		for key in value do
			if type(key) == "string" and isIdentifier(key) then
				table.insert(names, key)
			end
		end
	end
	return names
end

-- The completions of `word`, the dotted name the cursor is on (`config.pl`), among the `roots` (name to value) and the
-- shared globals. Each completion is a whole word that replaces `word`.
local function complete(roots, word)
	local head, fragment = string.match(word, "^(.-)([%w_]*)$")
	local found = {}
	local function offer(names, prefix)
		for _, name in names do
			if string.sub(name, 1, #fragment) == fragment and name ~= fragment then
				table.insert(found, prefix .. name)
			end
		end
	end
	if head == "" then
		offer(namesOf(roots), "")
		offer(namesOf(_G), "")
		offer(KEYWORDS, "")
	else
		local separator = string.sub(head, -1)
		if separator ~= "." and separator ~= ":" then
			return {}
		end
		local path = string.sub(head, 1, -2)
		local current = roots
		local first = true
		for step in string.gmatch(path, "[^.:]+") do
			if not isIdentifier(step) then
				return {}
			end
			if first then
				current = roots[step]
				if current == nil then
					current = _G[step]
				end
				first = false
			elseif type(current) == "table" then
				current = current[step]
			else
				return {}
			end
		end
		if first then
			return {}
		end
		offer(namesOf(current), head)
	end
	table.sort(found)
	local unique = {}
	for _, name in found do
		if unique[#unique] ~= name then
			table.insert(unique, name)
		end
		if #unique >= MAX_CANDIDATES then
			break
		end
	end
	return unique
end

-- `engine` as the console sees it: everything but `ecs`, whose declarations would change the schedule while it runs.
local function engineView(engine)
	local view = {}
	for key, value in engine do
		if key ~= "ecs" then
			view[key] = value
		end
	end
	return table.freeze(view)
end

return {
	format = format,
	complete = complete,
	engineView = engineView,
}
