
if not is_plat("macosx") then
    add_requires("vulkan-loader 1.4.x", {alias = "vulkan-loader"})
end

add_requires("vulkan-memory-allocator 3.3.x", {alias = "vulkan-memory-allocator"})
add_requires("vulkan-memory-allocator-hpp v3.3.0+3",
             {alias = "vulkan-memory-allocator-hpp", configs = {use_vulkanheaders = true}})
add_requires("stb", {alias = "stb"})
add_requires("imgui 1.92.x", {alias = "imgui", configs = {vulkan = true}})
add_requires("slang 2025.x", {alias = "slang", configs = {shared = true}})

rule("glsl.spirv")
    set_extensions(".vert", ".frag", ".comp", ".geom", ".tesc", ".tese")

    on_load(function (target)
        local outdir = path.join(target:targetdir(), "shaders")
        os.mkdir(outdir)
        target:data_set("shader_outdir", outdir)
    end)

    before_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        local glslang = target:data("glslang") or "glslangValidator"
        local outdir  = target:data("shader_outdir")

        local basename = path.basename(sourcefile)
        local stage    = path.extension(sourcefile):sub(2)
        local spv      = path.join(outdir, basename .. "_" .. stage .. ".spv")

        batchcmds:show_progress(opt.progress,
            "${color.build.object}compiling %s.%s to SPIR-V", basename, stage)
        batchcmds:mkdir(outdir)
        batchcmds:vrunv(glslang, {"-V", "-S", stage, sourcefile, "-o", spv})

        batchcmds:add_depfiles(sourcefile)
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
    add_packages("vulkan-headers", "vulkan-memory-allocator", "vulkan-memory-allocator-hpp", "glm", "stb", "imgui", "slang")
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
    add_files("shaders/*.vert", "shaders/*.frag", "shaders/*.comp", "shaders/*.geom", "shaders/*.tesc", "shaders/*.tese")
