-- A game with a plugin of its own, run by the generic runtime: `./aylo-runtime --server --game examples/games/plugin_demo/game`.
-- In a release build, its library must be signed (docs/Plugins.md).
includes("game/plugins/greeter")
