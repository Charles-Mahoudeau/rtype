add_requires("vulkan-headers 1.4.x", {alias = "vulkan-headers"})
add_requires("vulkan-loader 1.4.x", {alias = "vulkan-loader"})
add_requires("vulkan-memory-allocator 3.x", {alias = "vulkan-memory-allocator"})
add_requires("stb", {alias = "stb"})
add_requires("imgui 1.92.x", {alias = "imgui", configs = {glfw = true, vulkan = true}})
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
    add_deps("engine")
    add_packages("vulkan-headers", "vulkan-loader", "vulkan-memory-allocator", "glfw", "glm", "stb", "imgui", "slang")
    add_packages("vulkan-headers", {public = true})
    add_files("**.cpp")
    add_includedirs(".", {public = true})

    add_rules("glsl.spirv")
    add_files("shaders/*.vert", "shaders/*.frag", "shaders/*.comp", "shaders/*.geom", "shaders/*.tesc", "shaders/*.tese")
