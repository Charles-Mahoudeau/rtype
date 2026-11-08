-- The messages between the server and the clients that are not replicated state: joining a team, and the moments the
-- clients cannot see by themselves (only the server steps the physics, so only it knows when the ball is kicked or
-- bounces). Both sides declare them alike.

local Game = require("@scripts/shared/rules")

-- A client picks a team (Game.red, Game.blue, or Game.spectators to leave the pitch) and gives its name.
engine.net.packet("game.Join", {
	direction = "toServer",
	channel = "reliable",
	fields = { { "team", "u8" }, { "name", "string", Game.nameLength } },
})
-- A player kicked the ball, at the ball's position; `power` is the ball's speed after the kick.
engine.net.packet("game.Kicked", {
	direction = "toClients",
	channel = "unreliable",
	fields = { { "x", "f32" }, { "y", "f32" }, { "power", "f32" } },
})
-- The ball hit a post or a line, at the ball's position, at `speed`.
engine.net.packet("game.Bounce", {
	direction = "toClients",
	channel = "unreliable",
	fields = { { "x", "f32" }, { "y", "f32" }, { "speed", "f32" } },
})
-- A player dashed, from `x`, `y`.
engine.net.packet("game.Dashed", {
	direction = "toClients",
	channel = "unreliable",
	fields = { { "x", "f32" }, { "y", "f32" } },
})
-- A player took the power-up `kind`, at `x`, `y`.
engine.net.packet("game.PowerupTaken", {
	direction = "toClients",
	channel = "reliable",
	fields = { { "kind", "u8" }, { "x", "f32" }, { "y", "f32" } },
})
-- A goal for `team`, by `scorer` (empty when nobody touched the ball), `ownGoal` when the scorer is of the other team.
engine.net.packet("game.Goal", {
	direction = "toClients",
	channel = "reliable",
	fields = {
		{ "team", "u8" },
		{ "scorer", "string", Game.nameLength },
		{ "ownGoal", "bool" },
		{ "x", "f32" },
		{ "y", "f32" },
	},
})

return {}
