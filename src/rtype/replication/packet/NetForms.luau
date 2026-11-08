local forms = engine.forms

engine.net = {}

local types = { bool = true, u8 = true, u16 = true, u32 = true, i32 = true, f32 = true, string = true }
local schemas = {}

-- engine.net.packet(name, { direction = "toServer" | "toClients", channel = "reliable" | "unreliable", fields = { { name,
-- type, maxLength? }, ... } }): declares a packet, on both sides, while the scripts load. Returns its name.
engine.net.packet = function(...)
	local form = "engine.net.packet"
	forms.arguments(form, 2, ...)
	local name, given = ...
	if type(name) ~= "string" then
		forms.fail(form, "expected the packet's name first")
	end
	local spec = forms.table(form, given, { "direction", "channel", "fields" })
	local direction = forms.string(form, spec, "direction")
	local channel = forms.string(form, spec, "channel") or "reliable"
	if type(spec.fields) ~= "table" then
		forms.fail(form, "expected 'fields', a list of { name, type }")
	end
	local lines, list = {}, {}
	for index, field in ipairs(spec.fields) do
		local fieldName, fieldType, maxLength = field.name or field[1], field.type or field[2], field.maxLength or field[3]
		if type(fieldName) ~= "string" or not types[fieldType] then
			forms.fail(form, "field " .. index .. " of '" .. name .. "' needs a name and a type (bool, u8, u16, u32, i32, f32, string)")
		end
		if maxLength ~= nil and (fieldType ~= "string" or type(maxLength) ~= "number") then
			forms.fail(form, "only a string has a numeric maxLength ('" .. fieldName .. "')")
		end
		table.insert(lines, fieldName .. ":" .. fieldType .. (maxLength and (":" .. math.floor(maxLength)) or ""))
		table.insert(list, { name = fieldName, type = fieldType })
	end
	forms.call(form, "rtype.net.declarePacket", {
		name = name,
		direction = direction,
		channel = channel,
		fields = table.concat(lines, "\n"),
	})
	schemas[name] = list
	return name
end

-- Writes a table as the text the engine reads: for each field in order, `<size>:<bytes>`. The engine checks every
-- value again (ranges, lengths), so this only has to be well formed.
local function encode(form, name, values)
	local schema = schemas[name]
	if schema == nil then
		forms.fail(form, "unknown packet '" .. tostring(name) .. "' (declare it with engine.net.packet)")
	end
	if type(values) ~= "table" then
		forms.fail(form, "expected a table of values for '" .. name .. "'")
	end
	local parts = {}
	for _, field in ipairs(schema) do
		local value = values[field.name]
		local text
		if field.type == "string" then
			if type(value) ~= "string" then
				forms.fail(form, "'" .. name .. "." .. field.name .. "' must be a string")
			end
			text = value
		elseif field.type == "bool" then
			if type(value) ~= "boolean" then
				forms.fail(form, "'" .. name .. "." .. field.name .. "' must be a boolean")
			end
			text = value and "1" or "0"
		else
			if type(value) ~= "number" then
				forms.fail(form, "'" .. name .. "." .. field.name .. "' must be a number")
			end
			text = string.format("%.17g", value)
		end
		table.insert(parts, #text .. ":" .. text)
	end
	return table.concat(parts)
end

-- Reads what encode() writes (a received packet's `values`) back into a table.
local function decode(name, text)
	local schema = schemas[name]
	local result, position = {}, 1
	for _, field in ipairs(schema) do
		local colon = string.find(text, ":", position, true)
		local size = tonumber(string.sub(text, position, colon - 1))
		local raw = string.sub(text, colon + 1, colon + size)
		position = colon + size + 1
		if field.type == "string" then
			result[field.name] = raw
		elseif field.type == "bool" then
			result[field.name] = raw == "1"
		else
			result[field.name] = tonumber(raw)
		end
	end
	return result
end

-- engine.net.sendToServer(ctx, name, values): a client sends a `toServer` packet. `ctx` is a system's, which lists
-- "rtype.net.sendToServer" under `calls`. Returns whether it was sent.
engine.net.sendToServer = function(ctx, name, values)
	local form = "engine.net.sendToServer"
	local ok, result = pcall(ctx.call, "rtype.net.sendToServer", { name = name, values = encode(form, name, values) })
	if not ok then
		forms.fail(form, tostring(result))
	end
	return result == true
end

-- engine.net.sendToClients(ctx, name, values, client?): the server sends a `toClients` packet to every client, or to
-- `client`. `ctx` lists "rtype.net.sendToClients" under `calls`.
engine.net.sendToClients = function(ctx, name, values, client)
	local form = "engine.net.sendToClients"
	local ok, result = pcall(ctx.call, "rtype.net.sendToClients", {
		name = name,
		client = client or 0,
		values = encode(form, name, values),
	})
	if not ok then
		forms.fail(form, tostring(result))
	end
	return result == true
end

-- engine.net.received(ctx): the packets that arrived this frame, as a list of { name =, sender =, values = } where `values`
-- is a table of the packet's fields. The system lists `events = { read = { "net.PacketReceived" } }`. A packet this
-- script did not declare is skipped.
engine.net.received = function(ctx)
	local list = {}
	for packet in ctx.events.read("net.PacketReceived") do
		if schemas[packet.name] ~= nil then
			table.insert(list, { name = packet.name, sender = packet.sender, values = decode(packet.name, packet.values) })
		end
	end
	return list
end
