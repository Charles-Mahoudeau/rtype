-- The obstacles in the middle of the arena: two walls and a rock between them, static colliders that stop the lasers
-- and that the ships slide along.

-- The server's systems only: clients have none of the systems they are ordered against.
if not engine.isServer then
	return
end

local obstacles = {
	{ position = { x = Duel.middleX, y = 140 }, collider = { shape = Duel.box, size = { x = 40, y = 140 } } },
	{ position = { x = Duel.middleX, y = Duel.viewHeight / 2 }, collider = { shape = Duel.circle, radius = 36 } },
	{
		position = { x = Duel.middleX, y = Duel.viewHeight - 140 },
		collider = { shape = Duel.box, size = { x = 40, y = 140 } },
	},
}

-- Startup: spawns the obstacles, replicated through their Obstacle (the physics replicates the Collider).
engine.ecs.system("game.SpawnObstacles", {
	phase = "Startup",
	side = "server",
	query = {},
	run = function(ctx)
		for _, obstacle in obstacles do
			local collider = table.clone(obstacle.collider)
			collider.layer = Duel.obstacleLayer
			ctx.commands:spawn({
				["game.Obstacle"] = { position = obstacle.position },
				["rtype.Transform"] = { position = obstacle.position },
				["rtype.Collider"] = collider,
			})
		end
	end,
})
