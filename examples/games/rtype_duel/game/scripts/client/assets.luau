-- The client's assets: the sprite atlases, the sounds, the planets and the background. The forms load them; the
-- module returns their ids, for the client modules that use them.

return {
	-- The left sheet: 5 frames of 48x48, every 65 pixels.
	leftShip = engine.assets.atlas({
		name = "game.leftShip",
		texture = "r-typesheet24.gif",
		grid = { frameSize = { x = 48, y = 48 }, count = 5, spacing = { x = 17, y = 0 }, offset = { x = 9, y = 9 } },
		animations = { fly = { frames = { 0, 1, 2, 3, 4 }, frameDuration = 0.1 } },
	}),
	-- The right sheet: 3 frames of 63x48 facing left, every 65 pixels.
	rightShip = engine.assets.atlas({
		name = "game.rightShip",
		texture = "r-typesheet26.gif",
		grid = { frameSize = { x = 63, y = 48 }, count = 3, spacing = { x = 2, y = 0 }, offset = { x = 1, y = 1 } },
		animations = { fly = { frames = { 0, 1, 2 }, frameDuration = 0.1, direction = "pingPong" } },
	}),
	-- Two frames of 80x16 flying right from x 108, then two flying left from x 271, every 81 pixels.
	laser = engine.assets.atlas({
		name = "game.laser",
		texture = "bullet.gif",
		frames = {
			{ position = { x = 108, y = 151 }, size = { x = 80, y = 16 } },
			{ position = { x = 189, y = 151 }, size = { x = 80, y = 16 } },
			{ position = { x = 271, y = 151 }, size = { x = 80, y = 16 } },
			{ position = { x = 352, y = 151 }, size = { x = 80, y = 16 } },
		},
		animations = {
			right = { frames = { 0, 1 }, frameDuration = 0.08 },
			left = { frames = { 2, 3 }, frameDuration = 0.08 },
		},
	}),
	-- The big explosions, the bottom row: 10 frames of 64x64, every 65 pixels; the first six burst and fade, once.
	explosion = engine.assets.atlas({
		name = "game.explosion",
		texture = "r-fx-explosions.gif",
		grid = { frameSize = { x = 64, y = 64 }, count = 10, spacing = { x = 1, y = 0 }, offset = { x = 1, y = 99 } },
		animations = { explode = { frames = { 0, 1, 2, 3, 4, 5 }, frameDuration = 0.07, plays = 1 } },
	}),

	font = engine.assets.font("fonts/orbitron.json"),

	music = engine.assets.sound("maint.ogg", { stream = true }),
	laserSound = engine.assets.sound("laser.mp3"),
	misfireSound = engine.assets.sound("laser-no.mp3"),

	background = engine.assets.tilemap("levels/space-background.tmj"),
	planets = {
		{
			texture = engine.assets.texture("backgrounds/space/parallax-space-ring-planet.png"),
			size = { x = 51, y = 115 }, -- in pixels
		},
		{
			texture = engine.assets.texture("backgrounds/space/parallax-space-big-planet.png"),
			size = { x = 88, y = 87 },
		},
	},
}
