-- `mathx` is a library built in C++ (see main.cpp) and registered as a global: its functions are called like the
-- ones of the standard libraries.
print(`mathx.clamp(15, 0, 10) = {mathx.clamp(15, 0, 10)}`)
print(`mathx.clamp(-3, 0, 10) = {mathx.clamp(-3, 0, 10)}`)

local quotient, remainder = mathx.divide(17, 5)
print(`mathx.divide(17, 5) = {quotient}, remainder {remainder}`)

-- The library is a regular table, so it can be iterated over and its functions can be stored in locals.
local names = {}
for name in mathx do
	table.insert(names, name)
end
table.sort(names)
print(`mathx contains: {table.concat(names, ", ")}`)

-- A wrong call raises a Lua error before the C++ handler runs, which `pcall` catches.
local _, err = pcall(mathx.clamp, "15", 0, 10)
print(`mathx.clamp("15", 0, 10) failed: {err}`)
