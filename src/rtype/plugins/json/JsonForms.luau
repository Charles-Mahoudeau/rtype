local json = {}
engine.json = json

-- JSON for scripts (docs/Json.md). Pure Luau, run on every side the plugin is added to.
--   engine.json.encode(value) -> string      sorted keys, deterministic; errors on NaN/inf, functions, mixed tables
--   engine.json.decode(text) -> value        errors on malformed input, naming the byte; JSON null is nil
--   engine.json.tryDecode(text) -> value, nil | nil, message
--   engine.json.map({ { "a", 1 }, { "b", 2 } }) -> a table that encodes as an object with the keys in this order

local MAX_DEPTH = 64

local ESCAPES = { ['"'] = '\\"', ["\\"] = "\\\\", ["\b"] = "\\b", ["\f"] = "\\f", ["\n"] = "\\n", ["\r"] = "\\r", ["\t"] = "\\t" }
local UNESCAPES = { ['"'] = '"', ["\\"] = "\\", ["/"] = "/", b = "\b", f = "\f", n = "\n", r = "\r", t = "\t" }

-- The tables json.map marked. A weak set rather than a field, so that a table's own keys are never read as a marker.
local ordered = setmetatable({}, { __mode = "k" })

local function encodeString(s)
	return '"' .. string.gsub(s, '[%c"\\]', function(c)
		return ESCAPES[c] or string.format("\\u%04x", string.byte(c))
	end) .. '"'
end

local function encodeNumber(n)
	if n ~= n or n == math.huge or n == -math.huge then
		error("json: cannot encode " .. (n ~= n and "NaN" or n > 0 and "inf" or "-inf"), 0)
	end
	if n == math.floor(n) and math.abs(n) < 2 ^ 53 then
		return string.format("%d", n)
	end
	return string.format("%.17g", n)
end

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

local encodeValue

local function encodeTable(v, out, depth)
	if ordered[v] then
		out[#out + 1] = "{"
		for i, entry in ipairs(v) do
			if type(entry) ~= "table" or type(entry[1]) ~= "string" then
				error("json: map entry " .. i .. " must be { key, value } with a string key", 0)
			end
			if i > 1 then
				out[#out + 1] = ","
			end
			out[#out + 1] = encodeString(entry[1]) .. ":"
			encodeValue(entry[2], out, depth)
		end
		out[#out + 1] = "}"
	elseif isArray(v) then
		out[#out + 1] = "["
		for i = 1, #v do
			if i > 1 then
				out[#out + 1] = ","
			end
			encodeValue(v[i], out, depth)
		end
		out[#out + 1] = "]"
	else
		local keys = {}
		for k in pairs(v) do
			if type(k) ~= "string" then
				error("json: object keys must be strings, got a " .. type(k) .. " key (a table cannot mix array and object keys)", 0)
			end
			keys[#keys + 1] = k
		end
		table.sort(keys)
		out[#out + 1] = "{"
		for i, k in ipairs(keys) do
			if i > 1 then
				out[#out + 1] = ","
			end
			out[#out + 1] = encodeString(k) .. ":"
			encodeValue(v[k], out, depth)
		end
		out[#out + 1] = "}"
	end
end

function encodeValue(v, out, depth)
	local t = type(v)
	if v == nil then
		out[#out + 1] = "null"
	elseif t == "string" then
		out[#out + 1] = encodeString(v)
	elseif t == "number" then
		out[#out + 1] = encodeNumber(v)
	elseif t == "boolean" then
		out[#out + 1] = v and "true" or "false"
	elseif t == "table" then
		if depth >= MAX_DEPTH then
			error("json: nested too deeply to encode (more than " .. MAX_DEPTH .. " levels, or a table that contains itself)", 0)
		end
		encodeTable(v, out, depth + 1)
	else
		error("json: cannot encode a " .. t, 0)
	end
end

function json.encode(value)
	local out = {}
	encodeValue(value, out, 0)
	return table.concat(out)
end

function json.map(entries)
	if type(entries) ~= "table" then
		error("json: map expects a list of { key, value } entries", 0)
	end
	ordered[entries] = true
	return entries
end

local function fail(pos, message)
	error(string.format("json: %s at byte %d", message, pos), 0)
end

local function skip(text, pos)
	return string.find(text, "[^ \t\r\n]", pos) or #text + 1
end

local decodeValue

local function decodeString(text, pos)
	local out, i = {}, pos + 1
	while true do
		local j = string.find(text, '[\0-\31"\\]', i)
		if not j then
			fail(pos, "unterminated string")
		end
		out[#out + 1] = string.sub(text, i, j - 1)
		local c = string.sub(text, j, j)
		if c == '"' then
			return table.concat(out), j + 1
		end
		if c ~= "\\" then
			fail(j, "control character in string")
		end
		local e = string.sub(text, j + 1, j + 1)
		if e == "u" then
			local hex = string.match(text, "^%x%x%x%x", j + 2)
			if not hex then
				fail(j, "bad \\u escape")
			end
			local cp = tonumber(hex, 16)
			i = j + 6
			if cp >= 0xD800 and cp <= 0xDBFF then -- a high surrogate pairs with the \uDC00-\uDFFF escape after it
				local low = string.match(text, "^\\u(%x%x%x%x)", i)
				local lo = low and tonumber(low, 16)
				if lo and lo >= 0xDC00 and lo <= 0xDFFF then
					cp = 0x10000 + (cp - 0xD800) * 0x400 + (lo - 0xDC00)
					i += 6
				end
			end
			-- A lone surrogate is not text: it becomes U+FFFD.
			out[#out + 1] = (cp >= 0xD800 and cp <= 0xDFFF) and "\239\191\189" or utf8.char(cp)
		elseif UNESCAPES[e] then
			out[#out + 1] = UNESCAPES[e]
			i = j + 2
		else
			fail(j, "bad escape")
		end
	end
end

local function decodeNumber(text, pos)
	local int = string.match(text, "^-?%d+", pos)
	if not int then
		fail(pos, "unexpected character")
	end
	if string.match(int, "^-?0%d") then
		fail(pos, "leading zero in number")
	end
	local stop = pos + #int
	local frac = string.match(text, "^%.%d+", stop)
	if frac then
		stop += #frac
	end
	local exp = string.match(text, "^[eE][+-]?%d+", stop)
	if exp then
		stop += #exp
	end
	local n = tonumber(string.sub(text, pos, stop - 1))
	if not n or n ~= n or n == math.huge or n == -math.huge then
		fail(pos, "number out of range")
	end
	return n, stop
end

local function decodeObject(text, pos, depth)
	local obj = {}
	pos = skip(text, pos + 1)
	if string.sub(text, pos, pos) == "}" then
		return obj, pos + 1
	end
	while true do
		pos = skip(text, pos)
		if string.sub(text, pos, pos) ~= '"' then
			fail(pos, "expected a key")
		end
		local key
		key, pos = decodeString(text, pos)
		pos = skip(text, pos)
		if string.sub(text, pos, pos) ~= ":" then
			fail(pos, "expected ':'")
		end
		obj[key], pos = decodeValue(text, pos + 1, depth)
		pos = skip(text, pos)
		local d = string.sub(text, pos, pos)
		if d == "}" then
			return obj, pos + 1
		end
		if d ~= "," then
			fail(pos, "expected ',' or '}'")
		end
		pos += 1
	end
end

local function decodeArray(text, pos, depth)
	local arr = {}
	pos = skip(text, pos + 1)
	if string.sub(text, pos, pos) == "]" then
		return arr, pos + 1
	end
	while true do
		-- A null element is nil, so it takes no place in the array: the elements after it move down.
		arr[#arr + 1], pos = decodeValue(text, pos, depth)
		pos = skip(text, pos)
		local d = string.sub(text, pos, pos)
		if d == "]" then
			return arr, pos + 1
		end
		if d ~= "," then
			fail(pos, "expected ',' or ']'")
		end
		pos += 1
	end
end

function decodeValue(text, pos, depth)
	pos = skip(text, pos)
	local c = string.sub(text, pos, pos)
	if c == "{" or c == "[" then
		if depth >= MAX_DEPTH then
			fail(pos, "nested too deeply")
		end
		if c == "{" then
			return decodeObject(text, pos, depth + 1)
		end
		return decodeArray(text, pos, depth + 1)
	elseif c == '"' then
		return decodeString(text, pos)
	elseif string.sub(text, pos, pos + 3) == "true" then
		return true, pos + 4
	elseif string.sub(text, pos, pos + 4) == "false" then
		return false, pos + 5
	elseif string.sub(text, pos, pos + 3) == "null" then
		return nil, pos + 4
	end
	return decodeNumber(text, pos)
end

function json.decode(text)
	if type(text) ~= "string" then
		error("json: expected a string, got a " .. type(text), 0)
	end
	local value, pos = decodeValue(text, 1, 0)
	local rest = skip(text, pos)
	if rest <= #text then
		fail(rest, "trailing data")
	end
	return value
end

function json.tryDecode(text)
	local ok, value = pcall(json.decode, text)
	if ok then
		return value
	end
	return nil, tostring(value)
end
