-- The client's assets. The forms load them on the clients, and give their ids on the server too, without loading
-- anything there. The textures are not smoothed (`linear = false`): hard edges, even when the view is scaled.

return {
	-- The music, decoded while it plays (stream): a long track.
	music = engine.assets.sound("sounds/music.ogg", { stream = true }),
	-- The HUD's font, Orbitron (fonts/OFL.txt), a font atlas made with msdf-atlas-gen.
	font = engine.assets.font("fonts/orbitron.json"),
	-- The 8 roads, repeated down the view and scrolling on their own (the map's `scrolly`).
	roads = engine.assets.tilemap("levels/roads.tmj"),
	-- One square of a shot's beam, repeated from the shooter to what it hits (client/beams.luau).
	beam = engine.assets.texture("beam.png", { linear = false }),
	-- Its glow: a material of the glow shader (shaders/glow.glsl), a white core pulsing along the beam.
	beamGlow = engine.assets.material({
		name = "game.beamGlow",
		shader = engine.assets.shader("shaders/glow.glsl"),
		values = { core = 0.25, pulses = 25 },
	}),
	-- The looks of the traffic and of the riders (the cyclist's, tinted): the capsule files and the server name them in
	-- each game.Look, these forms load them.
	traffic = {
		engine.assets.texture("vehicles/car.png", { linear = false }),
		engine.assets.texture("vehicles/electric_car.png", { linear = false }),
		engine.assets.texture("vehicles/van.png", { linear = false }),
		engine.assets.texture("vehicles/truck.png", { linear = false }),
		engine.assets.texture("vehicles/bus.png", { linear = false }),
		engine.assets.texture("vehicles/limousine.png", { linear = false }),
		engine.assets.texture("cyclists/cyclist.png", { linear = false }),
	},
}
