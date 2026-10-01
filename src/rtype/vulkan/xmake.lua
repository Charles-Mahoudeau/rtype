-- macOS: Homebrew's loader (brew install molten-vk vulkan-loader), built to find Homebrew's MoltenVK driver and
-- layers. xmake's own loader is configured for its own prefix and would see neither.
if is_plat("macosx") then
    add_requires("pkgconfig::vulkan", {alias = "vulkan-loader"})
else
    add_requires("vulkan-loader 1.4.x", {alias = "vulkan-loader"})
end
add_requires("vulkan-memory-allocator 3.x", {alias = "vulkan-memory-allocator"})
add_requires("stb", {alias = "stb"})
add_requires("imgui 1.92.x", {alias = "imgui", configs = {vulkan = true}})
add_requires("slang 2025.x", {alias = "slang"})

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

target("vulkan")
    set_kind("shared")
    set_basename("rtype-vulkan")
    add_deps("engine-core")
    add_packages("vulkan-headers", "vulkan-loader", "vulkan-memory-allocator", "glm", "stb", "imgui", "slang")
    add_packages("vulkan-headers", {public = true})
    -- vk::raii::Context takes the linked vkGetInstanceProcAddr instead of dlopen()-ing its own loader. Public: it
    -- changes the layout of vk::raii::Context, so every file including the renderer headers must agree on it.
    add_defines("VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0", {public = true})

    if is_plat("macosx") then
        on_load(function (target)
            local brewprefix = try { function () return os.iorunv("brew", {"--prefix"}):trim() end }
            if brewprefix then
                target:add("rpathdirs", path.join(brewprefix, "lib"), {public = true})
            end
        end)
    end
    add_files("**.cpp")
    add_includedirs(".", {public = true})

    add_rules("glsl.spirv")
    add_files("shaders/*.vert", "shaders/*.frag", "shaders/*.comp", "shaders/*.geom", "shaders/*.tesc", "shaders/*.tese")
