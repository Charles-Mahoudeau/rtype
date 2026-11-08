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
	-- The looks of the riders, one per slot in the players' colors (blue, red, green, purple), and of the traffic: the
	-- server names them in each rider's game.Look and the capsule files in theirs, these forms load them.
	riders = {
		engine.assets.texture("riders/rider_0.png", { linear = false }),
		engine.assets.texture("riders/rider_1.png", { linear = false }),
		engine.assets.texture("riders/rider_2.png", { linear = false }),
		engine.assets.texture("riders/rider_3.png", { linear = false }),
	},
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
