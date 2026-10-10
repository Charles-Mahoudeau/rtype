
if not is_plat("macosx") then
    add_requires("vulkan-loader 1.4.x", {alias = "vulkan-loader"})
end

add_requires("vulkan-memory-allocator 3.3.x", {alias = "vulkan-memory-allocator"})
add_requires("vulkan-memory-allocator-hpp v3.3.0+3",
             {alias = "vulkan-memory-allocator-hpp", configs = {use_vulkanheaders = true}})
add_requires("stb", {alias = "stb"})
add_requires("imgui 1.92.x", {alias = "imgui", configs = {vulkan = true}})
add_requires("glslang 1.4.x", {alias = "glslang", configs = {binaryonly = true, spirv_tools = true}})

rule("glsl.spirv")
    set_extensions(".vert", ".frag", ".comp", ".geom", ".tesc", ".tese")

    before_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        import("lib.detect.find_tool")
        local glslang = find_tool("glslangValidator")
        assert(glslang, "glslangValidator not found: add_packages(\"glslang\") to the target")

        local outdir = path.join(target:targetdir(), "shaders")
        local spv = path.join(outdir, path.filename(sourcefile) .. ".spv")
        local includedir = path.join(path.directory(sourcefile), "include")

        local argv = {"-V", "--target-env", "vulkan1.3", "-I" .. includedir}
        table.insert(argv, is_mode("debug") and "-g" or "-Os")
        table.join2(argv, {"-o", spv, sourcefile})

        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.glsl %s", sourcefile)
        batchcmds:mkdir(outdir)
        batchcmds:vrunv(glslang.program, argv)

        batchcmds:add_depfiles(sourcefile)
        batchcmds:add_depfiles(os.files(path.join(includedir, "**")))
        batchcmds:set_depmtime(os.mtime(spv))
        batchcmds:set_depcache(target:dependfile(spv))
    end)
rule_end()

-- Vulkan renderer: IRenderer with Vulkan.
target("render-vulkan")
    set_kind("shared")
    set_basename("rtype-render-vulkan")
    -- engine-core for IRenderer, interop-vulkan for IVulkanSurfaceSource (implemented by the platforms).
    add_deps("engine-core", "interop-vulkan")
    add_packages("vulkan-headers", "vulkan-memory-allocator", "vulkan-memory-allocator-hpp", "glm", "stb", "imgui")
    add_packages("vulkan-headers", {public = true})
    -- vk::raii::Context takes the linked vkGetInstanceProcAddr instead of dlopen()-ing its own loader. Public: it
    -- changes the layout of vk::raii::Context, so every file including the renderer headers must agree on it.
    add_defines("VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0", {public = true})

    if is_plat("macosx") then
        -- Homebrew's loader (brew install molten-vk vulkan-loader): xmake's is configured for its own prefix and
        -- would see neither Homebrew's MoltenVK driver nor its layers. Only the library is linked: its headers
        -- are another version than vulkan-headers, and mixing both breaks vk::raii (VK_HEADER_VERSION mismatch).
        on_load(function (target)
            local loaderprefix = try { function () return os.iorunv("brew", {"--prefix", "vulkan-loader"}):trim() end }
            if not loaderprefix or not os.isdir(path.join(loaderprefix, "lib")) then
                raise("Vulkan loader not found: brew install molten-vk vulkan-loader")
            end
            target:add("linkdirs", path.join(loaderprefix, "lib"))
            target:add("links", "vulkan")
            -- Homebrew's layer manifests name their library without a path, and dyld does not search Homebrew's
            -- lib folder for bare names: add it to the rpath of every binary using the renderer.
            local brewprefix = os.iorunv("brew", {"--prefix"}):trim()
            target:add("rpathdirs", path.join(brewprefix, "lib"), {public = true})
        end)
    else
        add_packages("vulkan-loader")
    end
    add_defines("RTYPE_RENDER_VULKAN_BUILD", {public = false})
    add_files("**.cpp")
    -- Headers are included as "render/vulkan/...".
    add_includedirs("../..", {public = true})

    add_rules("glsl.spirv")
    add_packages("glslang")
    -- Only the stages that have shaders, so a missing one is not a warning on every build.
    for _, stage in ipairs({"vert", "frag", "comp", "geom", "tesc", "tese"}) do
        if #os.files(path.join(os.scriptdir(), "shaders", "*." .. stage)) > 0 then
            add_files("shaders/*." .. stage)
        end
    end
