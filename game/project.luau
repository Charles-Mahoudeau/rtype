if engine.isClient then
	engine.settings.platform = "glfw"
	engine.settings.renderer = "vulkan"

	engine.settings.window.title = "R-Type Duel"
	engine.settings.window.size = { 800, 600 }

	engine.settings.audio.backend = "miniaudio"
	engine.settings.audio.volume = {} -- FIXME: This need to be handled by C++
	engine.settings.audio.volume.music = 1.0

	engine.settings.vulkan.virtualResolution = { 800, 600 }

	if engine.isDebug then
		engine.settings.vulkan.layers = { "VK_LAYER_KHRONOS_validation" }
		engine.settings.vulkan.debugging = true
	end
end
