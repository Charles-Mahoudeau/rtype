-- The client's assets. The forms load them on the clients, and give their ids on the server too, without loading
-- anything there.

return {
	-- The 8 roads, repeated down the view and scrolling on their own (the map's `scrolly`).
	roads = engine.assets.tilemap("levels/roads.tmj"),
	-- One square of a shot's beam, repeated from the shooter to what it hits (client/beams.luau).
	beam = engine.assets.texture("beam.png"),
	-- The looks of the riders, one per slot in the players' colors (blue, red, green, purple), and of the traffic: the
	-- server names them in each rider's game.Look and the capsule files in theirs, these forms load them.
	riders = {
		engine.assets.texture("riders/rider_0.png"),
		engine.assets.texture("riders/rider_1.png"),
		engine.assets.texture("riders/rider_2.png"),
		engine.assets.texture("riders/rider_3.png"),
	},
	traffic = {
		engine.assets.texture("vehicles/car.png"),
		engine.assets.texture("vehicles/electric_car.png"),
		engine.assets.texture("vehicles/van.png"),
		engine.assets.texture("vehicles/truck.png"),
		engine.assets.texture("vehicles/bus.png"),
		engine.assets.texture("vehicles/limousine.png"),
		engine.assets.texture("cyclists/cyclist.png"),
	},
}
