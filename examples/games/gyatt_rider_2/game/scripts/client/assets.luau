-- The client's assets. The forms load them on the clients, and give their ids on the server too, without loading
-- anything there.

Game.assets = {
	-- The 8 roads, repeated down the view and scrolling on their own (the map's `scrolly`).
	roads = engine.assets.tilemap("levels/roads.tmj"),
}
