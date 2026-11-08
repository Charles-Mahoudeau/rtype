-- editor: the editor of a game. A static library, linked by the aylo-editor program (and by the tests), which adds its
-- plugin to the engine. It shows the game inside a window of docked panels (the game, its entities and their components, the frame statistics) made with
-- Dear ImGui, which lives here and nowhere else: the renderer only draws the meshes and the scene texture the contract
-- gives it (IRenderer::drawMesh(), setSceneSize()), and the graphics plugin lets it filter the window's events.

-- The docking branch of Dear ImGui, without any of its backends: the editor feeds it the contract's events and draws
-- its meshes through the contract's renderer.
add_requires("imgui v1.92.9+b-docking", {alias = "imgui"})

target("editor")
    -- Static, linked into the program: it links the plugins, so no plugin may link it.
    set_kind("static")
    set_basename("rtype-editor")
    add_defines("RTYPE_EDITOR_BUILD", {public = false})
    add_defines("RTYPE_EDITOR_STATIC", {public = true})
    add_deps("core", "ecs", "contract-platform", "contract-renderer", "plugin-graphics", "plugin-capsule", "plugin-particle", {public = true})
    -- The admin channel of the server the editor runs as a child process (a static module: the editor is linked into
    -- an executable, as the engine is).
    add_deps("network", {public = true})
    -- Headers are included as "editor/..." and "plugins/...".
    add_includedirs("..", {public = true})
    add_packages("glm", {public = true})
    add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
    -- Private: no header of the editor includes ImGui.
    add_packages("imgui")
    -- Not app/: the program's main().
    add_files("**.cpp|app/**.cpp")
    add_headerfiles("**.hpp", "**.tpp")

-- aylo-editor: the program that runs a game inside the editor, always as a debug run. The Engine loads the plugins and
-- backends from their libraries, as in aylo-runtime; the binary is copied to the repository root after the build.
target("aylo-editor")
    set_kind("binary")
    add_deps("engine", "editor")
    -- Built first, not linked: the Engine loads them.
    add_deps("plugin-libraries", "backend-libraries", {inherit = false})
    add_files("app/**.cpp")
    add_rules("rtype.copy_to_root")
    -- What `--version` prints: the version (with the nightly stamp) and the commit.
    on_load(function (target)
        import("engine_version")
        target:add("defines", 'RTYPE_ENGINE_VERSION="' .. engine_version.full() .. '"')
        target:add("defines", 'RTYPE_ENGINE_COMMIT="' .. engine_version.commit() .. '"')
    end)
