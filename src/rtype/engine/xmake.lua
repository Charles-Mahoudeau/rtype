-- Engine core: ECS, events and input. Depends on no window, graphics or scripting library,
-- so every other module can depend on it.
target("engine-core")
    set_kind("static")
    set_basename("rtype-engine-core")
    add_files("**.cpp|main.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    -- Headers are included as "engine/...".
    add_includedirs("..", {public = true})
    add_packages("glm", {public = true})
    -- Plain x/y/z/w members instead of unions (cppcoreguidelines-pro-type-union-access).
    add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
    -- Linked into the shared modules.
    add_cxflags("-fPIC", {tools = {"clang", "gcc"}})

-- The executable: wires the core with the platform, the renderer and the scripting.
target("engine")
    set_kind("binary")
    add_deps("engine-core", "platform", "luau", "vulkan")
    add_files("main.cpp")
    set_rundir("$(projectdir)")
