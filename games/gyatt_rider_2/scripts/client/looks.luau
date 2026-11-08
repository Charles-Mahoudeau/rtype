-- The look of everything the server moves, riders and traffic alike: the texture of its game.Look, at its look's
-- size, on its look's layer, where its game.Position says. What shows the cyclist's sheet plays its riding animation.

local assets = require("@scripts/client/assets")

-- Update: gives what the server spawned its sprite.
engine.ecs.system("game.DressLooks", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Position", "game.Look" }, without = { "rtype.SpriteRenderer" } },
	run = function(ctx)
		for entity, position, look in ctx.query do
			local size = look.size
			ctx.commands:add(entity, "rtype.Transform", { position = position.value })
			ctx.commands:add(entity, "rtype.SpriteRenderer", {
				texture = look.texture,
				source = { x = 0, y = 0, z = size.x, w = size.y }, -- the whole image
				origin = { x = size.x / 2, y = size.y / 2 }, -- centred on its position, like its hitbox
				layer = look.layer,
			})
			if look.texture == assets.cyclistSheet then
				ctx.commands:add(entity, "rtype.SpriteAnimator", { atlas = assets.cyclist, animation = "ride" })
			end
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

-- Update, after the positions the server sent: the vehicles go up at their speed between two of their positions, which
-- the server sends only now and then.
engine.ecs.system("game.DriveLooks", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Speed" }, write = { "rtype.Transform" } },
	after = { "game.FollowPositions" },
	before = { "game.FillGauge" },
	run = function(ctx)
		for _, speed, transform in ctx.query do
			if speed.value ~= 0 then
				local at = transform.position
				transform.position = { x = at.x, y = at.y - speed.value * ctx.time.delta }
			end
		end
	end,
})
