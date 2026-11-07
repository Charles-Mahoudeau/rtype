local forms = engine.forms
local form = "engine.http.request"

engine.http = {}

-- Returns the headers table as `Name: value` lines, sorted by name so that a request is always written the same way.
local function readHeaders(value)
	if value == nil then
		return nil
	end
	local lines = {}
	for _, entry in forms.named(form .. ": 'headers'", value) do
		if type(entry[2]) ~= "string" then
			forms.fail(form .. ": header '" .. entry[1] .. "'", "must be a string")
		end
		table.insert(lines, entry[1] .. ": " .. entry[2])
	end
	return table.concat(lines, "\n")
end

-- engine.http.request(ctx, { url, method, headers, body, timeout }): sends a request and returns its id, which the
-- rtype.HttpResponse event carries. `ctx` is a system's (which lists "rtype.http.request" under `calls`), or nil
-- while the script loads.
engine.http.request = function(...)
	forms.arguments(form, 2, ...)
	local ctx, given = ...
	if ctx ~= nil and type(ctx) ~= "table" then
		forms.fail(form, "expected a system's ctx (or nil) first, then the request")
	end
	local spec = forms.table(form, given, { "url", "method", "headers", "body", "timeout" })
	local url = forms.string(form, spec, "url")
	if url == nil or url == "" then
		forms.fail(form, "expected a 'url'")
	end
	local fields = {
		url = url,
		method = forms.string(form, spec, "method"),
		headers = readHeaders(spec.headers),
		body = forms.string(form, spec, "body"),
		timeout = forms.number(form, spec, "timeout"),
	}
	if ctx == nil then
		return forms.call(form, "rtype.http.request", fields)
	end
	local results = table.pack(pcall(ctx.call, "rtype.http.request", fields))
	if not results[1] then
		forms.fail(form, tostring(results[2]))
	end
	return results[2]
end
