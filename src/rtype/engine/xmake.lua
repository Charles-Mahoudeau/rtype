-- Engine core: ECS, events and input. Depends on no window, graphics or scripting library.
target("engine")
    set_kind("static")
    set_basename("rtype-engine")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "engine/...".
    add_includedirs("..", {public = true})
    add_packages("glm", {public = true})
    -- Plain x/y/z/w members instead of unions (cppcoreguidelines-pro-type-union-access).
    add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
    -- Linked into the shared modules.
    add_cxflags("-fPIC", {tools = {"clang", "gcc"}})
