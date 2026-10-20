-- Keep the package's defaults: its debug_renderer option does not reach Jolt's CMake (the library is always built with
-- JPH_DEBUG_RENDERER), and a define that differs from the library's makes JPH::RegisterTypes() abort.
add_requires("joltphysics 5.6.x", {alias = "joltphysics"})

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
