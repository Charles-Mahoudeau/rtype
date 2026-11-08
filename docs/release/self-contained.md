# Self-contained releases

A release of the engine runs on a machine with nothing installed but the operating system and the GPU driver: no
compiler, no development package, no runtime to download. A game ships the `runtime/` folder of the release as it is
(see [the plan](plan.md), D2), so the rule is about every binary in that folder.

## The rule

Every executable and library of `runtime/` finds what it links **in the folder**, or among what the operating system
provides. Nothing else is allowed:

| Platform | Allowed outside the folder |
|---|---|
| Linux | glibc (`libc`, `libm`, `libdl`, `libpthread`, `librt`, `ld-linux`), and `libvulkan.so.1`, which comes with the GPU driver |
| Windows | the system DLLs (`kernel32`, `user32`, `ucrtbase`, `api-ms-win-*`...), and `vulkan-1.dll` |
| macOS | `/usr/lib/*` and `/System/Library/*` (`libSystem`, `libc++`, the frameworks) |

The C++ runtime is **not** on the list: `libstdc++.so.6` and `libgcc_s.so.1` (Linux) and the Visual C++ runtime
(`vcruntime140*.dll`, `msvcp140*.dll`) are not on a machine without a compiler or its redistributable, so the release
holds them. A search path (`RUNPATH`, `@rpath`) that leaves the folder, such as a build directory, or a symbolic link
to a file outside it, is also an offender.

## The check

```sh
.github/scripts/check-self-contained.sh <folder>      # Linux and macOS
.github/scripts/check-self-contained.ps1 <folder>     # Windows
```

It reads the dynamic dependencies of every binary of the folder (`readelf` for ELF, `otool` for Mach-O, the import
tables for PE: no tool of Visual Studio), prints one offender per line on the error output and exits with 1, or with 0
when the folder is self-contained. `SELF_CONTAINED_ALLOW` adds names to the allowlist, for a check on a folder that
deliberately relies on one. `.github/scripts/test-check-self-contained.sh` tests it on small folders made on the spot.

A library opened at run time (`dlopen`, `LoadLibrary`) does not show in the dependencies, which is why the allowlist
names the ones we know: the Vulkan loader, and the window system libraries GLFW opens (X11, Wayland).

## How the build gets there

- **Packages are static and come from xmake.** `libsodium`, the SPIRV-Tools of glslang, `libcurl` and its OpenSSL are
  declared with `system = false`: xmake builds them, instead of linking the shared library of the machine that builds
  the release. A new dependency is added the same way (`add_requires`, never a system library), and stays private to
  the module that wraps it. A dependency of a package does not inherit `system = false` from a root requirement of the
  same name: it takes its own `add_requireconfs` (`vma-hpp.vulkan-headers`, `libcurl.openssl3`).
- **The engine's own modules are static.** `luau`, `network`, `replication`, `scripting` and `engine` are linked into
  `aylo-runtime` (and `aylo-sign`), so the release holds their code once, with the packages they wrap (`libsodium`,
  Asio, Luau), and has five libraries fewer to bundle and to find. `ecs` and the contracts stay shared, as do the
  plugins and the backends. The build refuses a shared library that links a static module (`rtype.static_modules`).
- **GLFW is linked alone on Linux** (`platform-glfw`). It opens X11 and Wayland itself, but the xmake package lists
  them and thirty `libxcb-*` libraries as links, which the build trims to `glfw3` before linking.
- **The Vulkan loader is opened, not linked** (`render-vulkan`, `core::Loader`), on Linux and Windows: the loader comes
  with the GPU driver, and a machine without one gets `Vulkan loader not found (tried libvulkan.so.1, libvulkan.so):
  install the driver of your graphics card` instead of a library that refuses to load. VMA is told not to bind the Vulkan
  functions itself (`VMA_STATIC_VULKAN_FUNCTIONS=0`): the allocator takes them from the dispatchers of the Vulkan
  instance and device.
- **Every binary has relative search paths only**, set by the build (rules `rtype.layout` and `rtype.plugin_library`):
  a plugin `$ORIGIN` (the plugins it links) and `$ORIGIN/../lib`; a backend `$ORIGIN/../../lib`; a library of `lib/`
  `$ORIGIN`, and the engine also `$ORIGIN/../plugins`, where the transform and input plugins it links are; an executable
  `$ORIGIN/lib` and `$ORIGIN/plugins`. Nothing looks above `runtime/`. A search path is not inherited by the libraries a
  binary loads, so each one carries its own. The executable of the build also finds the build folder from the
  repository root (`aylo-runtime` is copied there); the package replaces its search paths with those of the zip.
- **Windows uses the dynamic C runtime** (`/MD`, `/MDd`), shared by every library since C++ types cross their boundaries.

## What the packaging adds

The build does not put these in the folder; the packaging task (REL-04) does, and the check proves it did:

- **Linux:** `libstdc++.so.6` and `libgcc_s.so.1`, found with `<compiler> -print-file-name=libstdc++.so.6`, copied next
  `lib/`, with `patchelf --set-rpath '$ORIGIN'` (they have no search path of their own, and `libstdc++`
  needs `libgcc_s`). Bundled rather than linked statically: each library would otherwise hold its own copy of the C++
  runtime, and the exceptions and type information that cross them would not agree. The release is built on an old
  glibc (an Ubuntu 22.04 container), as a binary runs on any glibc at least as recent as the one it was linked against.
- **Windows:** the Visual C++ runtime DLLs of the toolchain (`Microsoft.VC*.CRT` of `%VCToolsRedistDir%`, or of the
  Visual Studio `vswhere` finds), next to the executable.
- **macOS:** the Vulkan loader and MoltenVK of Homebrew (`libvulkan.1.dylib`, `libMoltenVK.dylib`) in `lib/`, their
  install names set to `@rpath/...`, and MoltenVK's driver manifest in `runtime/vulkan/icd.d/` (library path
  `../../lib/libMoltenVK.dylib`): the loader searches the resources of the main bundle, which for an executable outside
  a bundle is its own folder. Every binary then names the bundled libraries by `@rpath` and loses its search paths
  outside the folder (the renderer's `/opt/homebrew/lib`), with `install_name_tool`, before the signature.

The package holds real files only: the build makes `lib/` and `plugins/` of real files, and the check reports a
symbolic link that leaves the folder.

## State

Verified on Linux: the `runtime/` folder of the package (`lib/`, `plugins/`, `plugins/backends/` of real files, the C++
runtime added as above) passes the check, and its server runs a game with only glibc loaded from outside the
folder. Verified on macOS: the package passes the check and the smoke test, and its client loads the bundled MoltenVK
through the bundled loader (`VK_LOADER_DEBUG=driver` shows `runtime/vulkan/icd.d/` searched first). The Windows part
(`.ps1`, `/MD`, the bundled C++ runtime) runs in the release workflow only.
