-- Plugins: the engine's features as ECS plugins (input, later the window and the renderer). Each plugin keeps its
-- code and declares its own components, resources and events. The only target that sees both the ECS and the engine
-- core, so neither depends on the other.
target("plugins")
    set_kind("static")
    set_basename("rtype-plugins")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    -- Headers are included as "plugins/...".
    add_includedirs("..", {public = true})
    add_deps("ecs", "engine-core", {public = true})
