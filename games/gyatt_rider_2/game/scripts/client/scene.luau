-- The scene around the game: the camera and the strip on the left (a scene), and the roads scrolling down as the riders
-- race up (a map).

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")

local roadsDrawLayer = -100 -- under every sprite
local sidebarColor = { x = 0.8, y = 0.82, z = 0.85, w = 1 }

-- Startup: loads the scene (assets/scenes/roads.scene): the places of the camera and of the strip on the left of the
-- roads. Its nodes are the client's, so the server spawns nothing of it.
engine.ecs.system("game.LoadScene", {
	phase = "Startup",
	side = "client",
	query = {},
	calls = { "rtype.capsule.load" },
	run = function(ctx)
		engine.capsule.load(ctx, Game.roadsScene)
	end,
})

-- Update: gives the scene's camera marker its camera, once the scene is spawned.
engine.ecs.system("game.DressScene", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Camera" }, without = { "rtype.Camera" } },
	run = function(ctx)
		for entity in ctx.query do
			ctx.commands:add(entity, "rtype.Camera", {})
		end
	end,
})

-- Update: gives the scene's sidebar marker its rectangle.
engine.ecs.system("game.DressSidebar", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Sidebar" }, without = { "rtype.ShapeRenderer" } },
	run = function(ctx)
		for entity in ctx.query do
			ctx.commands:add(entity, "rtype.ShapeRenderer", {
				kind = Game.rectShape,
				size = { x = Game.roadsLeft, y = Game.viewHeight },
				fillColor = sidebarColor,
			})
		end
	end,
})

-- Startup: the roads, a map whose image layer repeats down the view and scrolls on its own.
engine.ecs.system("game.SpawnRoads", {
	phase = "Startup",
	side = "client",
	query = {},
	calls = { "rtype.tilemap.spawn" },
	run = function(ctx)
		ctx.call("rtype.tilemap.spawn", {
			map = assets.roads,
			position = { x = 0, y = 0 },
			scale = { x = 1, y = 1 },
			firstDrawLayer = roadsDrawLayer,
		})
	end,
})
