-- Haxball: a top-down football match, up to 4 against 4. The view is 1280x720 world units, the pitch fills it.
local GAME_WIDTH = 1280
local GAME_HEIGHT = 720

-- The scripts: the shared entry runs first on both sides, then the entry of the side (see scripts/).
engine.settings.scripts.main = "shared/main"
engine.settings.scripts.server = "server/main"
engine.settings.scripts.client = "client/main"

-- The network, from the command line: `--port 4243` on both sides, `--host 10.0.0.2` on a client.
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end

-- Seen from above: the physics without gravity. F3 outlines every collider (hidden at first).
engine.settings.plugins = { "rtype.Physics" }
if engine.isClient then
	engine.settings.plugins = {
		"rtype.Physics", "rtype.Input", "rtype.graphics", "rtype.Shader", "rtype.Sprite", "rtype.SpriteDraw",
		"rtype.Shape", "rtype.Text", "rtype.Particle", "rtype.audio", "rtype.Ui", "rtype.PhysicsDebug",
	}
end
engine.settings.physics.gravity = { 0, 0 }

if engine.isClient then
	engine.settings.physicsDebug.enabled = false
	engine.settings.physicsDebug.drawAll = true

	engine.settings.platform = "glfw"
	engine.settings.renderer = "vulkan"
	engine.settings.window.system = engine.args.windowSystem or "auto"

	engine.settings.window.title = "Haxball"
	engine.settings.window.size = { GAME_WIDTH, GAME_HEIGHT }
	engine.settings.window.resizable = true

	engine.settings.audio.backend = "miniaudio"
	engine.settings.audio.volume.music = 0.35 -- under the effects

	engine.settings.vulkan.virtualResolution = { GAME_WIDTH, GAME_HEIGHT }

	if engine.isDebug then
		engine.settings.vulkan.layers = { "VK_LAYER_KHRONOS_validation" }
		engine.settings.vulkan.debugging = true
	end
end
