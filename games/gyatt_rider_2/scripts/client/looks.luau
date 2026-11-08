-- The look of everything the server moves, riders and traffic alike: the texture of its game.Look, at its
-- collider's size, on its look's layer, where its game.Position says.

-- Update: gives what the server spawned its sprite.
engine.ecs.system("game.DressLooks", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Position", "game.Look", "rtype.Collider" }, without = { "rtype.SpriteRenderer" } },
	run = function(ctx)
		for entity, position, look, collider in ctx.query do
			local size = collider.size
			ctx.commands:add(entity, "rtype.Transform", { position = position.value })
			ctx.commands:add(entity, "rtype.SpriteRenderer", {
				texture = look.texture,
				source = { x = 0, y = 0, z = size.x, w = size.y }, -- the whole image
				origin = { x = size.x / 2, y = size.y / 2 }, -- centred on its position, like its collider
				layer = look.layer,
			})
		end
	end,
})

-- Update: the sprites of what moved follow the positions the server sends.
engine.ecs.system("game.FollowPositions", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Position" }, write = { "rtype.Transform" }, changed = { "game.Position" } },
	run = function(ctx)
		for _, position, transform in ctx.query do
			transform.position = position.value
		end
	end,
})
