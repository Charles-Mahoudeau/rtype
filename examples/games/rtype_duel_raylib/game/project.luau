local GAME_WIDTH = 800
local GAME_HEIGHT = 600

-- The scripts: the shared entry runs first on both sides, then the entry of the side (see scripts/).
engine.settings.scripts.main = "shared/main"
engine.settings.scripts.server = "server/main"
engine.settings.scripts.client = "client/main"

-- The network, from the command line: `aylo-runtime --server <this folder> --port 4243`,
-- `aylo-runtime <this folder> --host 10.0.0.2`.
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end

-- A top-down arena: the physics without gravity, and F3 outlines every collider (hidden at first).
-- The arena and what the server spawns in it are capsules (assets/scenes, assets/actors).
engine.settings.plugins = { "rtype.Physics", "rtype.Capsule" }
if engine.isClient then
	engine.settings.plugins = {
		"rtype.Physics", "rtype.Capsule", "rtype.Input", "rtype.graphics", "rtype.Shader", "rtype.Sprite",
		"rtype.SpriteDraw", "rtype.Shape", "rtype.Tilemap", "rtype.Text", "rtype.Particle", "rtype.audio",
		"rtype.PhysicsDebug",
	}
end
-- `aylo-editor <this folder>` opens the arena to edit it; `--open actors/ship_left.actor` opens
-- another capsule (an actor opens alone).
engine.settings.editor.scene = engine.args.open or "scenes/arena.scene"
engine.settings.physics.gravity = { 0, 0 }
engine.settings.physicsDebug.enabled = false
engine.settings.physicsDebug.drawAll = true

if engine.isClient then
	-- The only difference with rtype_duel: the window, the input and the drawing go through raylib instead of GLFW and
	-- Vulkan. Not a line of the scripts changes.
	engine.settings.platform = "raylib"
	engine.settings.renderer = "raylib"

	engine.settings.window.title = "R-Type Duel (raylib)"
	engine.settings.window.size = { GAME_WIDTH, GAME_HEIGHT }

	engine.settings.audio.backend = "miniaudio"
	engine.settings.audio.volume.music = 1.0

	engine.settings.raylib.virtualResolution = { GAME_WIDTH, GAME_HEIGHT }
end
