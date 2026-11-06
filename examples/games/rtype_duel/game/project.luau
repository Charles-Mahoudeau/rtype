local GAME_WIDTH = 800
local GAME_HEIGHT = 600

-- The network, from the command line: `rtype_duel server --port 4243`, `rtype_duel client --host 10.0.0.2`.
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end

-- A top-down arena: the physics without gravity, and F3 outlines every collider (hidden at first).
engine.settings.plugins = { "rtype.Physics" }
if engine.isClient then
	engine.settings.plugins = {
		"rtype.Physics", "rtype.Input", "rtype.graphics", "rtype.Shader", "rtype.Sprite", "rtype.SpriteDraw",
		"rtype.Shape", "rtype.Tilemap", "rtype.Text", "rtype.Particle", "rtype.audio", "rtype.PhysicsDebug",
	}
end
engine.settings.physics.gravity = { 0, 0 }
engine.settings.physicsDebug.enabled = false
engine.settings.physicsDebug.drawAll = true

if engine.isClient then
	engine.settings.platform = "glfw"
	engine.settings.renderer = "vulkan"

	engine.settings.window.title = "R-Type Duel"
	engine.settings.window.size = { GAME_WIDTH, GAME_HEIGHT }

	engine.settings.audio.backend = "miniaudio"
	engine.settings.audio.volume.music = 1.0

	engine.settings.vulkan.virtualResolution = { GAME_WIDTH, GAME_HEIGHT }

	if engine.isDebug then
		engine.settings.vulkan.layers = { "VK_LAYER_KHRONOS_validation" }
		engine.settings.vulkan.debugging = true
	end
end
