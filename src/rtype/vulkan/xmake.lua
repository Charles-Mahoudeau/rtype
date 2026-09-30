add_requires("vulkan-headers 1.4.x", {alias = "vulkan-headers"})
add_requires("vulkan-loader 1.4.x", {alias = "vulkan-loader"})
add_requires("vulkan-memory-allocator 3.x", {alias = "vulkan-memory-allocator"})
add_requires("stb", {alias = "stb"})
add_requires("imgui 1.92.x", {alias = "imgui", configs = {vulkan = true}})
add_requires("slang 2025.x", {alias = "slang"})
if is_plat("macosx") then
    add_requires("moltenvk", {alias = "moltenvk", configs = {shared = true}})
end

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
    add_files("**.cpp")
    add_includedirs(".", {public = true})

    if is_plat("macosx") then
        after_build(function (target)
            import("core.base.json")
            import("core.project.project")

            local moltenvk = project.required_package("moltenvk")
            local searchdirs = table.wrap(moltenvk:get("linkdirs"))
            if moltenvk:installdir() then
                table.insert(searchdirs, path.join(moltenvk:installdir(), "lib"))
            end
            local library
            for _, dir in ipairs(searchdirs) do
                local candidate = path.join(dir, "libMoltenVK.dylib")
                if os.isfile(candidate) then
                    library = path.absolute(candidate)
                    break
                end
            end
            if not library then
                raise("libMoltenVK.dylib not found in the moltenvk package (searched: %s)", table.concat(searchdirs, ", "))
            end

            local icddir = path.join(target:targetdir(), "vulkan", "icd.d")
            os.mkdir(icddir)
            json.savefile(path.join(icddir, "MoltenVK_icd.json"), {
                file_format_version = "1.0.0",
                ICD = {library_path = library, api_version = "1.4.0", is_portability_driver = true}
            })
        end)
    end

    add_rules("glsl.spirv")
    add_files("shaders/*.vert", "shaders/*.frag", "shaders/*.comp", "shaders/*.geom", "shaders/*.tesc", "shaders/*.tese")
