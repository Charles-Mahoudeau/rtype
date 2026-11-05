-- A component and a system, declared from a file the ScriptingPlugin loads.

engine.ecs.component("game.Shield", {
    fields = { strength = "f32", regen = "f32" },
    defaults = { strength = 10, regen = 5 },
    presence = "both", -- the server simulates it and the clients have it
})
