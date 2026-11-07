-- The obstacles in the middle of the arena come from its scene (assets/scenes/arena.scene): two walls and a rock, static
-- colliders that stop the lasers and that the ships slide along. The server tells the clients where they are.

local Duel = require("@scripts/shared/arena")

-- The server's systems only, and only with the network: run alone, the clients' systems see the Transform itself.
if not engine.isServer or not Duel.networked then
	return
end

-- PostUpdate, once the gameplay moved what it moves: an obstacle the arena spawned replicates its place, for the clients
-- to draw it there.
engine.ecs.system("game.TrackObstacles", {
	phase = "PostUpdate",
	side = "server",
	query = { read = { "rtype.Transform" }, write = { "game.Obstacle" }, added = { "game.Obstacle" } },
	before = { "net.snapshot" },
	run = function(ctx)
		for _, transform, obstacle in ctx.query do
			obstacle.position = transform.position
		end
	end,
})
