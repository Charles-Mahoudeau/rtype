-- The scene: the camera on the middle of the view, the pitch under everything, and the pitch's colliders placed where
-- the server says (F3 outlines them).

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")

local pitchDrawLayer = -100 -- under every sprite

-- Startup: the camera, which hears too, and the pitch.
engine.ecs.system("game.SpawnScene", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		local middle = { x = Game.viewWidth / 2, y = Game.viewHeight / 2 }
		ctx.commands:spawn({
			["rtype.Transform"] = { position = middle },
			["rtype.Camera"] = {},
			["rtype.AudioListener"] = {}, -- a sound pans from where it happens
		})
		ctx.commands:spawn({
			["rtype.Transform"] = { position = middle },
			["rtype.SpriteRenderer"] = {
				texture = assets.pitch,
				source = { x = 0, y = 0, z = Game.viewWidth, w = Game.viewHeight },
				origin = middle,
				layer = pitchDrawLayer,
			},
		})
	end,
})

-- Update: the walls and posts the server spawned get their place, for the debug view.
engine.ecs.system("game.PlaceFixtures", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Fixture" }, without = { "rtype.Transform" } },
	run = function(ctx)
		for entity, fixture in ctx.query do
			ctx.commands:add(entity, "rtype.Transform", { position = fixture.position })
		end
	end,
})
