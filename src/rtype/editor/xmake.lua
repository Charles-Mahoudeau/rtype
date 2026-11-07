-- editor: the editor of a game, a plugin library the engine loads when `project.luau` lists "rtype.Editor". It shows the
-- game inside a window of docked panels (the game, its entities and their components, the frame statistics) made with
-- Dear ImGui, which lives here and nowhere else: the renderer only draws the meshes and the scene texture the contract
-- gives it (IRenderer::drawMesh(), setSceneSize()), and the graphics plugin lets it filter the window's events.

-- The docking branch of Dear ImGui, without any of its backends: the editor feeds it the contract's events and draws
-- its meshes through the contract's renderer.
add_requires("imgui v1.92.9+b-docking", {alias = "imgui"})

target("editor")
    set_kind("shared")
    set_basename("rtype-editor")
    add_defines("RTYPE_EDITOR_BUILD", {public = false})
    add_rules("rtype.plugin_library")
    add_deps("core", "ecs", "contract-platform", "contract-renderer", "plugin-graphics", "plugin-capsule", "plugin-particle", {public = true})
    -- Headers are included as "editor/..." and "plugins/...".
    add_includedirs("..", {public = true})
    add_packages("glm", {public = true})
    add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
    -- Private: no header of the editor includes ImGui.
    add_packages("imgui")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
