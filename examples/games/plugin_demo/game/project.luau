-- The plugin of game/plugins/greeter, named here like any plugin of the engine.
-- The game runs `scripts/main.luau` (the default entry).
engine.settings.plugins = { "demo.Greeter" }
engine.settings.network.port = engine.args.port or 4242
