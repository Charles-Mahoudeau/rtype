-- Luau -> C++
-- `cpp_divide` is implemented in C++ (see main.cpp): it takes two numbers, and returns the quotient and the
-- remainder of their division.
local quotient, remainder = cpp_divide(17, 5)
print(`Luau: 17 / 5 = {quotient}, remainder {remainder}`)

-- `cpp_greet` takes a name and an optional title: the title can be left out or be `nil`.
print(cpp_greet("Ada"))
print(cpp_greet("Ada", "Dr."))
print(cpp_greet("Ada", nil))

-- A wrong call raises a Lua error before the C++ handler runs, which `pcall` catches.
local ok, err = pcall(cpp_greet, "Ada", 42)
print(`Luau: cpp_greet("Ada", 42) failed: {err}`)

-- Running it without pcall will make the script execution stop.
-- cpp_greet("Ada", 42)

-- C++ -> Luau
-- `apply_damage` is called from C++ (see main.cpp) once the script has run. It takes the health of an entity, the
-- damage it takes and whether it is shielded, and returns its remaining health and whether it is still alive.
function apply_damage(health: number, damage: number, shielded: boolean): (number, boolean)
	-- A shield absorbs half of the damage.
	local taken = if shielded then damage / 2 else damage
	local remaining = math.max(health - taken, 0)
	return remaining, remaining > 0
end
