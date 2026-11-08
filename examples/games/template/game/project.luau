-- The starting point of a game: a project with one counter, a window on the client and a network port.
-- Run it from the folder of the engine (the one with `runtime/`):
--   runtime/aylo-runtime --server templates/game        the server, no window
--   runtime/aylo-runtime templates/game                 a client, in a window
--   runtime/aylo-runtime templates/game --headless      a client without a window, to test the network

-- The scripts run from `scripts/main.luau`. For a game with a server and a client side, name an entry per side:
--   engine.settings.scripts.server = "server/main"   engine.settings.scripts.client = "client/main"

engine.settings.network.port = engine.args.port or 4242

if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"

	if engine.args.headless then
		-- The client sends its inputs to the server, so it needs the input plugin even with no window.
		engine.settings.plugins = { "rtype.Input" }
	else
		engine.settings.plugins = { "rtype.graphics" }
		engine.settings.platform = "glfw"
		engine.settings.renderer = "vulkan"
		engine.settings.window.title = "My game"
	end
end
