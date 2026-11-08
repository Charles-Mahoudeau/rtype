
add_requires("vulkan-memory-allocator 3.3.x", {alias = "vulkan-memory-allocator"})
-- vma-hpp: xmake-repo's vulkan-memory-allocator-hpp, headers only, without its dependencies. That package adds its
-- vulkan-headers in on_install, after the requirements are resolved: neither a requirement nor add_requireconfs reaches
-- it, and its test compiles against whatever headers the machine has (Ubuntu 22.04's libvulkan-dev, 1.3.204, lacks
-- vulkan/vulkan_hpp_macros.hpp, so the release build failed there). This package copies the headers and checks they
-- are there; render-vulkan, which uses them, adds vulkan-headers (the root xmake.lua's, never the system's) and
-- vulkan-memory-allocator itself.
package("vma-hpp")
    set_base("vulkan-memory-allocator-hpp")
    on_install(function (package)
        os.cp("include", package:installdir())
    end)
    on_test(function (package)
        assert(os.isfile(path.join(package:installdir("include"), "vk_mem_alloc.hpp")))
    end)
package_end()
add_requires("vma-hpp v3.3.0+3", {alias = "vulkan-memory-allocator-hpp", configs = {use_vulkanheaders = true}})
-- glslangValidator compiles the shaders: from the glslang package required by the root xmake.lua, or from Homebrew on
-- macOS (brew install glslang).

-- Embeds each shader (shaders/<name>.<stage>) as SPIR-V: generates <name>.<stage>.h, which defines
-- const uint32_t rtype_shader_<name>_<stage>[] (other characters than letters and digits become '_').
-- Shaders include from the include/ folder next to them, and from the folders of set_values("glsl.includedirs", ...).
rule("glsl.spirv")
    set_extensions(".vert", ".frag", ".comp", ".geom", ".tesc", ".tese")

    on_load(function (target)
        local headerdir = path.join(target:autogendir(), "shaders")
        os.mkdir(headerdir)
        target:add("includedirs", headerdir)
    end)

    before_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        import("lib.detect.find_tool")
        -- The glslang package's bin folder first: it is not on the PATH in every context (project generation, CI
        -- jobs that do not build). Then the PATH (Homebrew's on macOS).
        local paths = {}
        local glslangpkg = target:pkg("glslang")
        if glslangpkg then
            table.insert(paths, path.join(glslangpkg:installdir(), "bin"))
        end
        local glslang = find_tool("glslangValidator", {paths = paths})
        assert(glslang, "glslangValidator not found: add_packages(\"glslang\") to the target"
                        .. " (on macOS: brew install glslang)")

        local filename = path.filename(sourcefile)
        local outdir = path.join(target:autogendir(), "shaders")
        local header = path.join(outdir, filename .. ".h")
        local variable = "rtype_shader_" .. (filename:gsub("[^%w]", "_"))
        local includedir = path.join(path.directory(sourcefile), "include")

        local includedirs = table.join({includedir}, target:values("glsl.includedirs") or {})
        local argv = {"-V", "--target-env", "vulkan1.3"}
        for _, dir in ipairs(includedirs) do
            table.insert(argv, "-I" .. dir)
        end
        table.insert(argv, is_mode("debug") and "-g" or "-Os")
        table.join2(argv, {"--vn", variable, "-o", header, sourcefile})

        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.glsl %s", sourcefile)
        batchcmds:mkdir(outdir)
        batchcmds:vrunv(glslang.program, argv)

        batchcmds:add_depfiles(sourcefile)
        for _, dir in ipairs(includedirs) do
            batchcmds:add_depfiles(os.files(path.join(dir, "**")))
        end
        batchcmds:set_depmtime(os.mtime(header))
        batchcmds:set_depcache(target:dependfile(header))
    end)
rule_end()

-- Vulkan renderer: IRenderer with Vulkan.
target("render-vulkan")
    add_rules("rtype.plugin_library", {folder = "plugins/backends"})
    set_kind("shared")
    set_basename("rtype-render-vulkan")
    -- contract-renderer for IRenderer, interop-vulkan for IVulkanSurfaceSource (implemented by the platforms).
    add_deps("core", "contract-renderer", "interop-vulkan")
    add_packages("vulkan-headers", "vulkan-memory-allocator", "vulkan-memory-allocator-hpp", "glm", "stb")
    add_packages("vulkan-headers", {public = true})
    -- vk::raii::Context takes the entry point core/Loader.cpp gives it instead of dlopen()-ing its own loader. Public:
    -- it changes the layout of vk::raii::Context, so every file including the renderer headers must agree on it.
    add_defines("VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0", {public = true})
    -- Nothing of the renderer links the Vulkan loader (it is opened at run time, so a release does not need it): VMA
    -- must not bind the Vulkan functions itself. vma::raii::createAllocator gives it the ones of the dispatchers.
    add_defines("VMA_STATIC_VULKAN_FUNCTIONS=0", "VMA_DYNAMIC_VULKAN_FUNCTIONS=0")

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
    end
    add_defines("RTYPE_RENDER_VULKAN_BUILD", {public = false})
    add_files("**.cpp")
    -- Headers are included as "render/vulkan/...".
    add_includedirs("../..", {public = true})

    add_rules("glsl.spirv")
    -- The built-in draw shaders are the default shader: they include the shaders ABI of the renderer
    -- contract, which game shaders are assembled in too.
    set_values("glsl.includedirs", path.join(os.scriptdir(), "..", "..", "contract", "renderer", "shaders"))
    if not is_plat("macosx") then
        add_packages("glslang")
    end
    -- Only the stages that have shaders, so a missing one is not a warning on every build.
    for _, stage in ipairs({"vert", "frag", "comp", "geom", "tesc", "tese"}) do
        if #os.files(path.join(os.scriptdir(), "shaders", "*." .. stage)) > 0 then
            add_files("shaders/*." .. stage)
        end
    end
