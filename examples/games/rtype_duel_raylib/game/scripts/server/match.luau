-- Whether the match is over. The state is local to this module: the systems that need it require it and call its
-- functions, nobody else can write it.

local over = false

local Match = {}

--- @return Whether a laser already hit a ship.
function Match.isOver(): boolean
	return over
end

--- Ends the match: the ships stop moving and no other hit wins.
function Match.finish()
	over = true
end

return table.freeze(Match)
