-- No debug renderer: the server never draws, and the client's debug view draws from the components.
add_requires("joltphysics 5.6.x", {alias = "joltphysics", configs = {debug_renderer = false}})

target("physics")
    set_kind("shared")
    set_basename("rtype-physics")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("RTYPE_PHYSICS_BUILD", {public = false})
    -- Headers are included as "rtype/physics/...".
    add_includedirs("$(projectdir)/src", {public = true})
    add_deps("ecs", "plugins")
    add_packages("glm", {public = true})
    -- Private: no public header includes Jolt, so its users never see it.
    add_packages("joltphysics", {public = false})
