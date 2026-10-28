-- Core: what several modules share and that belongs to none of them (the configuration for now), one folder per
-- feature. It depends on no other project target.
target("core")
    set_kind("static")
    set_basename("rtype-core")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    -- Headers are included as "core/...".
    add_includedirs("..", {public = true})
    -- Linked into the shared modules (render-vulkan).
    add_cxflags("-fPIC", {tools = {"clang", "gcc"}})
