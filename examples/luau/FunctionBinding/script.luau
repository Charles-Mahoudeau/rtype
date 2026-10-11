-- This is a C++ function:
hello_from_cpp()

-- This will be called from C++:
function hello_from_luau(): void
	print("Hello from Luau!")
end

-- This will be called from C++ too, with four arguments, and returns three values to C++:
-- the remaining health of the entity, whether it is still alive, and a report of the hit.
function apply_damage(name: string, health: number, damage: number, shielded: boolean): (number, boolean, string)
	-- A shield absorbs half of the damage.
	local taken = if shielded then damage / 2 else damage
	-- Health never goes below zero.
	local remaining = math.max(health - taken, 0)
	local alive = remaining > 0
	local report = string.format("%s took %g damage (%g -> %g)", name, taken, health, remaining)
	return remaining, alive, report
end
