-- A plugin of the game: built into game/plugins/, where the Engine finds it.
target("demo-greeter")
    set_kind("shared")
    set_default(false)
    set_basename("demo-greeter")
    add_deps("core", "ecs")
    add_files("Greeter.cpp")
    add_rules("rtype.game_plugin")
