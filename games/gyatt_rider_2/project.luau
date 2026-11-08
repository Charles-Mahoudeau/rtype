-- The view: a 1-tile strip on the left (the timer and the gauge), then 8 roads of 3 tiles, 64 pixels a tile, and as
-- high as 16:9 makes it.
local GAME_WIDTH = 1600
local GAME_HEIGHT = 900

-- The scripts: the shared entry runs first on both sides, then the entry of the side (see scripts/).
engine.settings.scripts.main = "shared/main"
engine.settings.scripts.server = "server/main"
engine.settings.scripts.client = "client/main"

-- The network, from the command line: `--port 4243` on both sides, `--host 10.0.0.2` on a client.
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end

-- Seen from above (the riders, the traffic and the scene around them are capsules, in assets/): the physics without
-- gravity, and F3 outlines every collider (hidden at first). The clients draw the HUD with the UI.
engine.settings.plugins = { "rtype.Physics", "rtype.Capsule" }
if engine.isClient then
	engine.settings.plugins = {
		"rtype.Physics", "rtype.Capsule", "rtype.Input", "rtype.graphics", "rtype.Shader", "rtype.Sprite",
		"rtype.SpriteDraw", "rtype.Shape", "rtype.Tilemap", "rtype.Text", "rtype.Ui", "rtype.audio",
		"rtype.PhysicsDebug",
	}
end
engine.settings.physics.gravity = { 0, 0 }
-- Room for many bodies: a client builds one for each vehicle that comes and never steps its world, and Jolt frees the
-- room of the removed ones only when the world steps, so a client ran out of it after a minute or two of traffic and
-- aborted. This only delays it, by hours: the engine's physics plugin must free it on clients (engine issue).
engine.settings.physics.maxBodies = 65536

if engine.isClient then
	engine.settings.physicsDebug.enabled = false
	engine.settings.physicsDebug.drawAll = true

	engine.settings.platform = "glfw"
	engine.settings.renderer = "vulkan"

	-- A window you can resize, or the whole screen with `--fullscreen`: the view is scaled to it, with bars where the
	-- screen's shape is not 16:9.
	engine.settings.window.title = "GyattRider 2"
	engine.settings.window.size = { GAME_WIDTH, GAME_HEIGHT }
	engine.settings.window.resizable = true
	engine.settings.window.fullscreen = engine.args.fullscreen == true

	engine.settings.vulkan.virtualResolution = { GAME_WIDTH, GAME_HEIGHT }

	-- The music, through miniaudio; a machine without a sound device plays silently rather than not starting.
	engine.settings.audio.backend = "miniaudio"
	engine.settings.audio.required = false
	engine.settings.audio.volume.music = 0.7

	if engine.isDebug then
		engine.settings.vulkan.layers = { "VK_LAYER_KHRONOS_validation" }
		engine.settings.vulkan.debugging = true
	end
end
