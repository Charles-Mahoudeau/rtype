-- The scene around the game: the camera, the roads scrolling down as the riders race up, and the strip on the left.

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")

local roadsDrawLayer = -100 -- under every sprite
local sidebarColor = { x = 0.8, y = 0.82, z = 0.85, w = 1 }

-- Startup: the camera looks at the middle of the view.
engine.ecs.system("game.SpawnCamera", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		ctx.commands:spawn({
			["rtype.Transform"] = { position = { x = Game.viewWidth / 2, y = Game.viewHeight / 2 } },
			["rtype.Camera"] = {},
		})
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

-- Startup: the strip on the left of the roads, where the timer and the gauge will go.
engine.ecs.system("game.SpawnSidebar", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		ctx.commands:spawn({
			["rtype.Transform"] = { position = { x = Game.roadsLeft / 2, y = Game.viewHeight / 2 } },
			["rtype.ShapeRenderer"] = {
				kind = Game.rectShape,
				size = { x = Game.roadsLeft, y = Game.viewHeight },
				fillColor = sidebarColor,
			},
		})
	end,
})
