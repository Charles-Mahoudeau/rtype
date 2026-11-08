local json = {}
engine.json = json

-- JSON for scripts (docs/Json.md): a thin Luau layer over the engine calls rtype.json.encode / rtype.json.decode
-- (JsonCalls.hpp), which do the real work in C++ with nlohmann::json. This file only inspects a Luau value's shape
-- (is a table an array, a plain object, or a json.map() ordered object?) on the way in, and rebuilds a Luau value
-- from the flattened node stream the calls take and return on the way out; it has no JSON grammar, no string
-- escaping and no number formatting of its own.
--   engine.json.encode(value) -> string      sorted keys, deterministic; errors on NaN/inf, functions, mixed tables
--   engine.json.decode(text) -> value        errors on malformed input, naming the byte; JSON null is nil
--   engine.json.tryDecode(text) -> value, nil | nil, message
--   engine.json.map({ { "a", 1 }, { "b", 2 } }) -> a table that encodes as an object with the keys in this order

local MAX_DEPTH = 64

-- Node kinds of the flattened stream rtype.json.encode takes and rtype.json.decode returns: rtype::plugins::json::NodeKind
-- (JsonCalls.hpp). Keep in the same order.
local NULL, FALSE, TRUE, NUMBER, STRING, ARRAY, OBJECT, ORDERED_OBJECT = 0, 1, 2, 3, 4, 5, 6, 7

-- The tables json.map marked. A weak set rather than a field, so that a table's own keys are never read as a marker.
local ordered = setmetatable({}, { __mode = "k" })

-- A table of 1..n with no hole (and no other key) is an array; the empty table is the empty array.
local function isArray(t)
	local n = 0
	for _ in pairs(t) do
		n += 1
	end
	for i = 1, n do
		if t[i] == nil then
			return false
		end
	end
	return true
end

-- flattenValue writes its stream into a table of four parallel lists plus a text pool (kinds, numbers, counts,
-- text, lengths), filled in pre-order; see rtype::plugins::json::NodeKind (JsonCalls.hpp) for how they read back.

local function pushText(stream, s)
	stream.text[#stream.text + 1] = s
	stream.lengths[#stream.lengths + 1] = #s
end

local flattenValue

local function flattenTable(v, depth, stream)
	if depth >= MAX_DEPTH then
		error("json: nested too deeply to encode (more than " .. MAX_DEPTH .. " levels, or a table that contains itself)", 0)
	end
	if ordered[v] then
		stream.kinds[#stream.kinds + 1] = ORDERED_OBJECT
		stream.counts[#stream.counts + 1] = #v
		for i, entry in ipairs(v) do
			if type(entry) ~= "table" or type(entry[1]) ~= "string" then
				error("json: map entry " .. i .. " must be { key, value } with a string key", 0)
			end
			pushText(stream, entry[1])
			flattenValue(entry[2], depth + 1, stream)
		end
	elseif isArray(v) then
		stream.kinds[#stream.kinds + 1] = ARRAY
		stream.counts[#stream.counts + 1] = #v
		for i = 1, #v do
			flattenValue(v[i], depth + 1, stream)
		end
	else
		local keys = {}
		for k in pairs(v) do
			if type(k) ~= "string" then
				error("json: object keys must be strings, got a " .. type(k) .. " key (a table cannot mix array and object keys)", 0)
			end
			keys[#keys + 1] = k
		end
		stream.kinds[#stream.kinds + 1] = OBJECT
		stream.counts[#stream.counts + 1] = #keys
		-- C++ sorts a plain object's keys before writing them: their order here does not matter.
		for _, k in keys do
			pushText(stream, k)
			flattenValue(v[k], depth + 1, stream)
		end
	end
end

function flattenValue(v, depth, stream)
	local t = type(v)
	if v == nil then
		stream.kinds[#stream.kinds + 1] = NULL
	elseif t == "boolean" then
		stream.kinds[#stream.kinds + 1] = v and TRUE or FALSE
	elseif t == "number" then
		stream.kinds[#stream.kinds + 1] = NUMBER
		stream.numbers[#stream.numbers + 1] = v
	elseif t == "string" then
		stream.kinds[#stream.kinds + 1] = STRING
		pushText(stream, v)
	elseif t == "table" then
		flattenTable(v, depth, stream)
	else
		error("json: cannot encode a " .. t, 0)
	end
end

-- Strips the "native function 'call': engine call '...': " prefix an engine call's error carries, so a json error
-- always reads "json: ...".
local function callError(message)
	return (string.gsub(tostring(message), "^.-engine call '[^']*': ", ""))
end

function json.encode(value)
	local stream = { kinds = {}, numbers = {}, counts = {}, text = {}, lengths = {} }
	flattenValue(value, 0, stream)
	local ok, result = pcall(engine.call, "rtype.json.encode", {
		kinds = stream.kinds,
		numbers = stream.numbers,
		counts = stream.counts,
		text = table.concat(stream.text),
		textLengths = stream.lengths,
	})
	if not ok then
		error(callError(result), 0)
	end
	-- A result with one field is that field's value: the text.
	return result
end

function json.map(entries)
	if type(entries) ~= "table" then
		error("json: map expects a list of { key, value } entries", 0)
	end
	ordered[entries] = true
	return entries
end

-- Reads the result of rtype.json.decode one node at a time, in the same order flattenValue wrote it.
local function makeReader(result)
	local cursors = { kind = 1, number = 1, count = 1, length = 1, text = 1 }

	local function nextText()
		local len = result.stringLengths[cursors.length]
		cursors.length += 1
		local s = string.sub(result.strings, cursors.text, cursors.text + len - 1)
		cursors.text += len
		return s
	end

	local read

	local function readArray()
		local count = result.counts[cursors.count]
		cursors.count += 1
		local arr = {}
		for _ = 1, count do
			-- A null element is nil, so it takes no place in the array: the elements after it move down.
			local v = read()
			if v ~= nil then
				arr[#arr + 1] = v
			end
		end
		return arr
	end

	local function readObject()
		local count = result.counts[cursors.count]
		cursors.count += 1
		local obj = {}
		for _ = 1, count do
			local key = nextText()
			obj[key] = read()
		end
		return obj
	end

	function read()
		local kind = result.kinds[cursors.kind]
		cursors.kind += 1
		if kind == NULL then
			return nil
		elseif kind == FALSE then
			return false
		elseif kind == TRUE then
			return true
		elseif kind == NUMBER then
			local n = result.numbers[cursors.number]
			cursors.number += 1
			return n
		elseif kind == STRING then
			return nextText()
		elseif kind == ARRAY then
			return readArray()
		elseif kind == OBJECT then
			return readObject()
		end
		error("json: malformed decode result: unknown node kind " .. tostring(kind), 0)
	end

	return read
end

function json.decode(text)
	if type(text) ~= "string" then
		error("json: expected a string, got a " .. type(text), 0)
	end
	local ok, result = pcall(engine.call, "rtype.json.decode", { text = text })
	if not ok then
		error(callError(result), 0)
	end
	return makeReader(result)()
end

function json.tryDecode(text)
	local ok, value = pcall(json.decode, text)
	if ok then
		return value
	end
	return nil, tostring(value)
end
