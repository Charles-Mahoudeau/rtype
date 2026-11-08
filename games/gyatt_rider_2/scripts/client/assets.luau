-- The client's assets. The forms load them on the clients, and give their ids on the server too, without loading
-- anything there. The textures are not smoothed (`linear = false`): hard edges, even when the view is scaled.

local rideFrameSeconds = 0.12 -- how long each frame of the riding animation shows

return {
	-- The music, decoded while it plays (stream): a long track.
	music = engine.assets.sound("sounds/music.ogg", { stream = true }),
	-- The HUD's font, Orbitron (fonts/OFL.txt), a font atlas made with msdf-atlas-gen.
	font = engine.assets.font("fonts/orbitron.json"),
	-- The 8 roads, repeated down the view and scrolling on their own (the map's `scrolly`).
	roads = engine.assets.tilemap("levels/roads.tmj"),
	-- The text box every big text sits on (client/textbox.luau).
	textBox = engine.assets.texture("text_box.png", { linear = false }),
	-- A white square, tinted and stretched into the plain rectangles of the strip on the left (client/blocks.luau).
	white = engine.assets.texture("white.png", { linear = false }),
	-- One square of a shot's beam, repeated from the shooter to what it hits (client/beams.luau).
	beam = engine.assets.texture("beam.png", { linear = false }),
	-- Its glow: a material of the glow shader (shaders/glow.glsl), a white core pulsing along the beam.
	beamGlow = engine.assets.material({
		name = "game.beamGlow",
		shader = engine.assets.shader("shaders/glow.glsl"),
		values = { core = 0.25, pulses = 25 },
	}),
	-- The cyclist, which the riders are too (tinted): its three frames side by side (cyclists/cyclist_sheet.png, packed
	-- by tools/pack_cyclist.py), riding 1 2 3 2 1... Its texture is the one game.Look names; client/looks.luau plays the
	-- atlas' animation on what shows it.
	cyclistSheet = engine.assets.texture("cyclists/cyclist_sheet.png", { linear = false }),
	cyclist = engine.assets.atlas({
		name = "game.cyclist",
		texture = "cyclists/cyclist_sheet.png",
		linear = false,
		grid = { frameSize = { x = 64, y = 128 }, count = 3 },
		animations = { ride = { frames = { 0, 1, 2 }, frameDuration = rideFrameSeconds, direction = "pingPong" } },
	}),
	rideFrameSeconds = rideFrameSeconds,
	-- The looks of the traffic: the capsule files name them in each game.Look, these forms load them.
	traffic = {
		engine.assets.texture("vehicles/car.png", { linear = false }),
		engine.assets.texture("vehicles/electric_car.png", { linear = false }),
		engine.assets.texture("vehicles/van.png", { linear = false }),
		engine.assets.texture("vehicles/truck.png", { linear = false }),
		engine.assets.texture("vehicles/bus.png", { linear = false }),
		engine.assets.texture("vehicles/limousine.png", { linear = false }),
	},
}
