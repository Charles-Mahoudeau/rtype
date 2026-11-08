-- Null backend: a platform and a renderer that open no window and draw nothing, so a client (UI, graphics, input
-- scripts) runs in tests and on a machine without a display. Name them in project.luau: platform = "null",
-- renderer = "null".
target("backend-null")
    add_rules("rtype.plugin_library", {folder = "plugins/backends"})
    set_kind("shared")
    set_basename("rtype-backend-null")
    add_deps("core", "contract-platform", "contract-renderer", {public = true})
    add_defines("RTYPE_BACKEND_NULL_BUILD", {public = false})
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "backend/null/...".
    add_includedirs("../..", {public = true})
