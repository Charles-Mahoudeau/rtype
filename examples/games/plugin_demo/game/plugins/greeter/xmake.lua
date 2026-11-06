-- A plugin of the game: built into game/plugins/, where the Engine finds it.
target("duel-greeter")
    set_kind("shared")
    set_default(false)
    set_basename("duel-greeter")
    add_deps("core", "ecs")
    add_files("Greeter.cpp")
    add_rules("rtype.game_plugin")
