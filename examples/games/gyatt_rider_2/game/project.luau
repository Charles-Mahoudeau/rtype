-- The view: a 1-tile strip on the left (the timer and the gauge), then 8 roads of 2 tiles, 64 pixels a tile.
local GAME_WIDTH = 1088
local GAME_HEIGHT = 640

-- The network, from the command line: `--port 4243` on both sides, `--host 10.0.0.2` on a client.
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end

-- Seen from above: the physics without gravity, and F3 outlines every collider (hidden at first).
engine.settings.plugins = { "rtype.Physics" }
if engine.isClient then
	engine.settings.plugins = {
		"rtype.Physics", "rtype.Input", "rtype.graphics", "rtype.Shader", "rtype.Sprite", "rtype.SpriteDraw",
		"rtype.Shape", "rtype.Tilemap", "rtype.PhysicsDebug",
	}
end
engine.settings.physics.gravity = { 0, 0 }

if engine.isClient then
	engine.settings.physicsDebug.enabled = false
	engine.settings.physicsDebug.drawAll = true

	engine.settings.platform = "glfw"
	engine.settings.renderer = "vulkan"

	engine.settings.window.title = "GyattRider 2"
	engine.settings.window.size = { GAME_WIDTH, GAME_HEIGHT }

	engine.settings.vulkan.virtualResolution = { GAME_WIDTH, GAME_HEIGHT }

	if engine.isDebug then
		engine.settings.vulkan.layers = { "VK_LAYER_KHRONOS_validation" }
		engine.settings.vulkan.debugging = true
	end
end
