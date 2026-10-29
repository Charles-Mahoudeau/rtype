local GAME_WIDTH = 800
local GAME_HEIGHT = 600

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
