-- `xmake package-release`: makes the zip game developers download, in build/package/. The layout is the one of
-- docs/release/plan.md and docs/release/package.md; the smoke test (.github/scripts/smoke-test.sh) checks the result.
--
--   xmake f -m release && xmake package-release [--no-manual] [--no-build]
--
-- A game ships the `runtime/` folder of the zip with its own `game/` folder.
task("package-release")
    set_menu {
        usage = "xmake package-release [options]",
        description = "Make the release zip of the engine in build/package (a release build)",
        options = {
            {nil, "no-build", "k", nil, "Do not build the runtime and the tools first"},
            {nil, "no-manual", "k", nil, "Leave the offline manual out of the zip"},
        },
    }

    on_run(function ()
        import("core.base.json")
        import("core.base.option")
        import("core.base.semver")
        import("core.project.config")
        import("lib.detect.find_tool")
        import("utils.archive")
        import("engine_version")

        config.load()
        local plat = config.plat()
        local windows = plat == "windows"
        local rootdir = os.projectdir()
        local libext = windows and ".dll" or (plat == "macosx" and ".dylib" or ".so")
        local exeext = windows and ".exe" or ""
        local osname = ({macosx = "macos"})[plat] or plat
        local arch = ({x64 = "x86_64", amd64 = "x86_64"})[config.arch()] or config.arch()

        if config.mode() ~= "release" then
            raise("the package is a release build: run `xmake f -m release` first (the mode is " .. tostring(config.mode()) .. ")")
        end
        if not option.get("no-build") then
            os.execv(os.programfile(), {"build", "-y", "-P", rootdir, "aylo-runtime", "aylo-sign"})
        end

        local builddir = path.join(path.absolute(config.builddir()), plat, config.arch(), config.mode())
        local version = engine_version.full()
        local name = format("aylo-engine-%s-%s-%s", version, osname, arch)
        local packagedir = path.join(path.absolute(config.builddir()), "package")
        local stage = path.join(packagedir, name)
        local runtime = path.join(stage, "runtime")
        local symbols = path.join(packagedir, name .. "-symbols")
        local zipfile = path.join(packagedir, name .. ".zip")
        local symbolszip = path.join(packagedir, name .. "-symbols.zip")

        for _, entry in ipairs({stage, symbols, zipfile, symbolszip}) do
            os.tryrm(entry)
        end
        os.mkdir(runtime)

        -- The executables, with their rpath relative to themselves.
        local function copyExecutable(from, to)
            if not os.isfile(from) then
                raise("%s is not built (looked for %s)", path.filename(from), from)
            end
            os.cp(from, to)
            if not windows then
                os.vrunv("chmod", {"755", to})
            end
        end
        copyExecutable(path.join(builddir, "aylo-runtime" .. exeext), path.join(runtime, "aylo-runtime" .. exeext))
        copyExecutable(path.join(builddir, "aylo-sign" .. exeext), path.join(stage, "tools", "aylo-sign" .. exeext))

        -- The libraries, as the build made them: `lib/` holds the ones that are not plugins, `plugins/` the plugins (real
        -- files, `plugins/backends/` for the backends). A library made for the tests is not part of the engine.
        local function isTestLibrary(file)
            return path.filename(file):find("test", 1, true) ~= nil
        end
        -- Windows has no rpath: what the executable links sits next to it, `lib/` included (the contracts and `ecs` are
        -- found there by the plugins too). The transform and input plugins are linked by the engine until it loads them,
        -- so they are there as well as in `plugins/`: a module of the same name is shared, so the plugin folder's copy is
        -- the one already loaded.
        local libdir = windows and runtime or path.join(runtime, "lib")
        os.mkdir(libdir)
        for _, file in ipairs(os.files(path.join(builddir, "lib", "*" .. libext))) do
            if not isTestLibrary(file) then
                os.cp(file, path.join(libdir, path.filename(file)))
            end
        end
        local pluginsdir = path.join(builddir, "plugins")
        for _, file in ipairs(os.files(path.join(pluginsdir, "**" .. libext))) do
            if not isTestLibrary(file) then
                local to = path.join(runtime, "plugins", path.relative(file, pluginsdir))
                os.mkdir(path.directory(to))
                os.cp(file, to)
                local filename = path.filename(file)
                if windows and (filename == "rtype-plugin-transform.dll" or filename == "rtype-plugin-input.dll") then
                    os.cp(file, path.join(runtime, filename))
                end
            end
        end
        -- Windows: the Visual C++ runtime of the toolchain (vcruntime140*.dll, msvcp140*.dll...), next to the executable,
        -- where the DLLs of plugins/ find it too (it is loaded once, by name). From %VCToolsRedistDir% when the build runs
        -- in a developer prompt, otherwise from the Visual Studio vswhere finds.
        if windows then
            local redistdir = os.getenv("VCToolsRedistDir")
            if not redistdir or not os.isdir(redistdir) then
                redistdir = nil
                local programfiles = os.getenv("ProgramFiles(x86)") or "C:\\Program Files (x86)"
                local vswhere = path.join(programfiles, "Microsoft Visual Studio", "Installer", "vswhere.exe")
                if os.isfile(vswhere) then
                    local installdir = try { function ()
                        return os.iorunv(vswhere, {"-latest", "-products", "*", "-requires",
                                                   "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                                   "-property", "installationPath"}):trim()
                    end }
                    -- Redist/MSVC/<version>/: the highest version is the one of the newest toolset.
                    local best
                    for _, dir in ipairs(installdir and os.dirs(path.join(installdir, "VC", "Redist", "MSVC", "*")) or {}) do
                        local name = path.filename(dir)
                        if name:match("^%d+%.%d+%.%d+$") and (not best or semver.compare(name, path.filename(best)) > 0) then
                            best = dir
                        end
                    end
                    redistdir = best
                end
            end
            local redistarch = ({x86_64 = "x64", arm64 = "arm64", x86 = "x86"})[arch] or arch
            local crt = redistdir and os.dirs(path.join(redistdir, redistarch, "Microsoft.VC*.CRT"))[1]
            if not crt then
                raise("the Visual C++ runtime was not found (no VCToolsRedistDir, nor a Visual Studio with the C++ tools): " ..
                      "the zip would not run where it is not installed")
            end
            for _, file in ipairs(os.files(path.join(crt, "*.dll"))) do
                os.cp(file, path.join(runtime, path.filename(file)))
            end
        end
        -- The tool finds the engine's DLLs next to itself.
        if windows then
            for _, file in ipairs(os.files(path.join(runtime, "*" .. libext))) do
                os.cp(file, path.join(stage, "tools", path.filename(file)))
            end
        end
        -- The C++ runtime of the machine that built it, in `lib/`: the zip must run where none is installed. They are the
        -- ones the executable resolves; their rpath is the folder, as theirs would be searched elsewhere otherwise. The
        -- search paths of the executable and the tool are set to the folders of the zip (the build's include its own
        -- folder, which does not exist here).
        if plat == "linux" then
            local patchelf = find_tool("patchelf")
            local resolved = os.iorunv("ldd", {path.join(runtime, "aylo-runtime")})
            for _, soname in ipairs({"libstdc++.so.6", "libgcc_s.so.1"}) do
                local found = resolved:match(soname:gsub("%p", "%%%0") .. " => (%S+)")
                if found and os.isfile(found) then
                    os.cp(found, path.join(libdir, soname))
                    if patchelf then
                        os.vrunv(patchelf.program, {"--set-rpath", "$ORIGIN", path.join(libdir, soname)})
                    else
                        cprint("${yellow}warning: patchelf is missing: %s is bundled without its rpath", soname)
                    end
                else
                    cprint("${yellow}warning: %s is not linked by the runtime: it is not bundled", soname)
                end
            end
            if patchelf then
                os.vrunv(patchelf.program, {"--set-rpath", "$ORIGIN/lib:$ORIGIN/plugins", path.join(runtime, "aylo-runtime")})
                os.vrunv(patchelf.program, {"--set-rpath", "$ORIGIN/../runtime/lib:$ORIGIN/../runtime/plugins",
                                            path.join(stage, "tools", "aylo-sign")})
            else
                cprint("${yellow}warning: patchelf is missing: the executables keep the search paths of the build")
            end
        end

        -- macOS has no Vulkan loader with the system: the renderer links Homebrew's (brew install molten-vk
        -- vulkan-loader), which the zip carries in `lib/` with MoltenVK, its driver. The loader looks for drivers in the
        -- resources of the main bundle, which for an executable outside a bundle is its own folder: the manifest of
        -- MoltenVK is `runtime/vulkan/icd.d/`, its library path relative to it. Every binary then names the bundled
        -- libraries by @rpath, and loses its search paths outside the folder (the renderer's /opt/homebrew/lib).
        if plat == "macosx" then
            local function brewprefix(formula)
                local prefix = try { function () return os.iorunv("brew", {"--prefix", formula}):trim() end }
                if not prefix or not os.isdir(prefix) then
                    raise("%s not found: brew install molten-vk vulkan-loader", formula)
                end
                return prefix
            end
            local libdir = path.join(runtime, "lib")
            local bundled = {}
            local function bundle(from, filename)
                local to = path.join(libdir, filename)
                -- os.cp copies the file a link points to, as a real file.
                os.cp(from, to)
                os.vrunv("chmod", {"755", to})
                os.vrunv("install_name_tool", {"-id", "@rpath/" .. filename, to})
                bundled[filename] = true
            end
            bundle(path.join(brewprefix("vulkan-loader"), "lib", "libvulkan.1.dylib"), "libvulkan.1.dylib")
            bundle(path.join(brewprefix("molten-vk"), "lib", "libMoltenVK.dylib"), "libMoltenVK.dylib")
            local icddir = path.join(runtime, "vulkan", "icd.d")
            os.mkdir(icddir)
            json.savefile(path.join(icddir, "MoltenVK_icd.json"), {
                file_format_version = "1.0.0",
                ICD = {library_path = "../../lib/libMoltenVK.dylib", api_version = "1.4.0", is_portability_driver = true},
            })

            local binaries = table.join(os.files(path.join(runtime, "**.dylib")),
                                        {path.join(runtime, "aylo-runtime"), path.join(stage, "tools", "aylo-sign")})
            for _, file in ipairs(binaries) do
                local args = {}
                -- otool -L: the first line is the file, then one dependency per line (the first of a library is its id).
                for line in os.iorunv("otool", {"-L", file}):gmatch("[^\n]+") do
                    local dependency = line:match("^%s+(%S+) %(compatibility")
                    if dependency and not dependency:startswith("@") and bundled[path.filename(dependency)] then
                        table.join2(args, {"-change", dependency, "@rpath/" .. path.filename(dependency)})
                    end
                end
                -- otool -l: an LC_RPATH command is followed by its `path <entry> (offset n)` line.
                for entry in os.iorunv("otool", {"-l", file}):gmatch("\n%s+path (%S+) %(offset") do
                    if not entry:startswith("@") then
                        table.join2(args, {"-delete_rpath", entry})
                    end
                end
                if #args > 0 then
                    table.insert(args, file)
                    os.vrunv("install_name_tool", args)
                end
            end
        end

        -- Licenses of what the runtime carries.
        local notices = path.join(rootdir, "xmake", "package", "THIRD_PARTY_NOTICES.md")
        os.mkdir(path.join(runtime, "licenses"))
        os.cp(notices, path.join(runtime, "licenses", "THIRD_PARTY_NOTICES.md"))

        -- The starting game.
        os.cp(path.join(rootdir, "examples", "games", "template", "game"), path.join(stage, "templates", "game"))

        -- The text files of the zip.
        local readme = io.readfile(path.join(rootdir, "xmake", "package", "README.md"))
        io.writefile(path.join(stage, "README.md"), (readme:gsub("@VERSION@", version)))
        io.writefile(path.join(stage, "VERSION"), version .. "\n")
        if os.isfile(path.join(rootdir, "LICENSE")) then
            os.cp(path.join(rootdir, "LICENSE"), path.join(stage, "LICENSE"))
        else
            cprint("${yellow}warning: the repository has no LICENSE file: the zip has none")
        end

        -- The offline manual.
        if option.get("no-manual") then
            cprint("${yellow}warning: --no-manual: the zip has no offline manual")
        else
            local uvx = find_tool("uvx")
            if uvx then
                os.vrunv(uvx.program, {"zensical", "build", "-f", path.join(rootdir, "manual", "mkdocs.yml")})
                os.cp(path.join(rootdir, "manual", "site"), path.join(stage, "manual"))
            else
                cprint("${yellow}warning: uvx is not installed: the zip has no offline manual")
            end
        end

        -- Strips what is not yet, keeping the debug information of what has some in the symbols zip.
        local function keepSymbols(file)
            if windows then
                return
            end
            if plat == "linux" then
                local objcopy, strip, readelf = find_tool("objcopy"), find_tool("strip"), find_tool("readelf")
                if not (objcopy and strip and readelf) then
                    cprint("${yellow}warning: binutils is missing: %s is not stripped", path.filename(file))
                    return
                end
                local sections = os.iorunv(readelf.program, {"-S", "--wide", file})
                local relative = path.relative(file, stage)
                if sections:find(".debug_info", 1, true) then
                    local debugfile = path.join(symbols, relative .. ".debug")
                    os.mkdir(path.directory(debugfile))
                    os.vrunv(objcopy.program, {"--only-keep-debug", file, debugfile})
                end
                if sections:find(".symtab", 1, true) or sections:find(".debug_", 1, true) then
                    os.vrunv(strip.program, {"--strip-unneeded", file})
                end
            end
        end
        for _, file in ipairs(table.join(os.files(path.join(stage, "runtime", "**" .. libext)),
                                          os.files(path.join(runtime, "aylo-runtime" .. exeext)),
                                          os.files(path.join(stage, "tools", "aylo-sign" .. exeext)))) do
            keepSymbols(file)
        end
        if windows then
            for _, pdb in ipairs(os.files(path.join(builddir, "*.pdb"))) do
                os.mkdir(symbols)
                os.cp(pdb, path.join(symbols, path.filename(pdb)))
            end
        end

        -- macOS: a stripped arm64 binary has an invalid signature and is killed, so every file is signed again, now that no
        -- other step changes it (the release workflow gives the script; no variable, no signature).
        local codesign = os.getenv("AYLO_CODESIGN_SCRIPT")
        if plat == "macosx" and codesign and #codesign > 0 then
            os.vrunv("bash", {codesign, stage})
        end

        -- The runtime must carry everything it needs: a script lists what it resolves outside its folder.
        local checkdir = path.join(rootdir, ".github", "scripts")
        local check = windows and path.join(checkdir, "check-self-contained.ps1") or path.join(checkdir, "check-self-contained.sh")
        if os.isfile(check) then
            if windows then
                os.vrunv("powershell", {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", check, runtime})
            else
                os.vrunv("bash", {check, runtime})
            end
        else
            cprint("${yellow}warning: %s does not exist: the runtime was not checked for dependencies outside it", path.relative(check, rootdir))
        end

        -- The manifest: what the runtime says about itself, which must be the version of this package, and a hash of every file.
        local report = os.iorunv(path.join(runtime, "aylo-runtime" .. exeext), {"--version"})
        local function field(key)
            local value = report:match(key .. ": ([^\r\n]+)")
            return value and value:trim()
        end
        local reported = report:match("^aylo%-engine ([^\r\n]+)")
        if reported ~= version then
            raise("the runtime says it is version %s, the package is %s", tostring(reported), version)
        end
        local files = {}
        for _, file in ipairs(os.files(path.join(stage, "**"))) do
            table.insert(files, {path = path.relative(file, stage):gsub("\\", "/"), sha256 = hash.sha256(file)})
        end
        table.sort(files, function (a, b) return a.path < b.path end)
        json.savefile(path.join(stage, "manifest.json"), {
            name = "aylo-engine",
            version = version,
            base_version = engine_version.base(),
            commit = field("commit"),
            platform = osname,
            arch = arch,
            toolchain = field("toolchain"),
            library_abi = tonumber(field("library ABI")),
            files = files,
        })

        -- The zips: the folder inside has the name of the package, so it extracts to one folder.
        -- On Linux and macOS, `zip -y` keeps the links of plugins/ as links (archive.archive would store a second copy of each
        -- library, which the engine would load twice).
        local function makeZip(file, folder)
            if windows then
                archive.archive(file, folder, {curdir = packagedir, compress = "best"})
            else
                local zip = find_tool("zip")
                if not zip then
                    raise("zip is not installed")
                end
                os.vrunv(zip.program, {"-q", "-r", "-y", "-X", "-9", file, folder}, {curdir = packagedir})
            end
        end
        makeZip(zipfile, name)
        print("package: %s", path.relative(zipfile, rootdir))
        if os.isdir(symbols) then
            makeZip(symbolszip, path.filename(symbols))
            print("symbols: %s", path.relative(symbolszip, rootdir))
        else
            print("no debug symbols in this build: no -symbols zip")
        end
    end)
task_end()
