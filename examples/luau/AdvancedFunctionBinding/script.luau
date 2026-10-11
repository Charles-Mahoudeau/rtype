-- Luau -> C++
-- `cpp_reverse` is implemented in C++ (see main.cpp): it is called like any Luau function, receives its arguments
-- and returns them in reverse order. Every type C++ can read is passed: nil, boolean, number, vector and string.
local s, v, n, b, z = cpp_reverse(nil, true, 42, vector.create(1, 2, 3), "text")
print("Luau got back from C++:", s, v, n, b, z)

-- An error raised by the C++ handler is a regular Luau error, which `pcall` catches.
local _, handlerError = pcall(cpp_reverse)
print("Luau caught:", handlerError)

-- An argument C++ cannot read yet (here, a table) raises an error before the handler even runs.
local _, argumentError = pcall(cpp_reverse, {})
print("Luau caught:", argumentError)

-- C++ -> Luau
-- `luau_reverse` is called from C++ (see main.cpp) once the script has run: it does the same as `cpp_reverse`, so
-- both directions can be compared.
function luau_reverse(...: any): ...any
	local count = select("#", ...)
	if count == 0 then
		error("luau_reverse expects at least one argument", 0)
	end
	local args = table.pack(...)
	local reversed = {}
	for i = 1, count do
		local value = args[i]
		local description = if value == nil then "nil" else `{typeof(value)} {value}`
		print(`Luau received argument #{i}: {description}`)
		reversed[count - i + 1] = value
	end
	-- `table.unpack` with an explicit count keeps the nil values.
	return table.unpack(reversed, 1, count)
end
