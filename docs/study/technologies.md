# Technologies

Each section compares the realistic options against [our constraints](index.md#our-constraints) (C1 to C8), then says
what we chose and what it costs.

## Language: C++23

C++ is imposed by the subject. The question was which standard.

| Option | For | Against |
|---|---|---|
| C++17 | Supported everywhere | No concepts, no `std::span`, no `std::expected`, no ranges: more templates, more code |
| C++20 | Concepts, `std::span`, ranges, `consteval` | No `std::expected`, fewer ranges |
| **C++23** | Everything above, plus the C++23 library and language additions | Library support still uneven between the three standard libraries: we use `tl::expected`, which has the API of `std::expected` |

**Choice: C++23**, built with Clang and Apple Clang, and kept compiling with MSVC (and GCC). Concepts and a runtime
description of each type (`ecs::TypeInfo`) remove most of the template machinery an ECS and a serializer need.

## Build system and packages: xmake

| Option | Build | Packages | For | Against |
|---|---|---|---|---|
| CMake + vcpkg | CMake | vcpkg (manifest) | The industry standard, every IDE | Two tools and two languages; slow configure; vcpkg triplets per platform |
| CMake + Conan | CMake | Conan 2 | Binary cache, profiles | Python tool to install; a second configuration language |
| CMake + CPM | CMake | Source fetch at configure | One tool | Builds every dependency from source in the build tree; no binary cache |
| Meson + WrapDB | Meson | Wraps | Fast, clean language | Fewer packages; Windows less smooth |
| **xmake** | xmake | **xrepo** (built in) | **One tool and one language (Lua) for build and packages**, ~1500 packages, per-package options (`configs`), cross-platform toolchains, compile commands for clang-tidy | Smaller community than CMake; some IDEs need a plugin |

**Choice: xmake.** A dependency is one `add_requires` line next to the target that uses it, with its options; a module
rule (`rtype.static_modules`) enforces the dependency rules at build time. Packages are built from source into a
per-user cache, never installed on the system (C1). The CI caches that folder between runs.

## Graphics API: Vulkan

| Option | For | Against |
|---|---|---|
| SFML / raylib (2D) | Fastest start | Their own window and input; a fixed pipeline we cannot extend (particles on the GPU, custom shaders) |
| OpenGL 3.3 / 4.x | Simple, everywhere | Deprecated on macOS (stuck at 4.1), global state machine (against C6), no compute on macOS |
| Direct3D 12 / Metal | Native, fast | One platform each |
| WebGPU (Dawn, wgpu) | Modern, portable | Young C++ API, heavy to build in CI |
| **Vulkan** | Explicit, modern, compute shaders, validation layers, runs on Linux, Windows, and macOS through MoltenVK | Verbose; more code before the first triangle |

**Choice: Vulkan**, behind the renderer contract (`contract/render/IRenderer`). The GPU particle system
([`particles/design.md`](../particles/design.md)) and user shaders ([`Shaders.md`](../Shaders.md)) rely on compute
and on our own pipelines. The renderer is a backend: another one (OpenGL, Metal) could be added without touching a
plugin. The Vulkan Memory Allocator (VMA) handles GPU memory, glslang compiles GLSL to SPIR-V.

## Windowing and input: GLFW

| Option | For | Against |
|---|---|---|
| **GLFW** | Small, only window + input + Vulkan surface, zlib license | One gamepad API (no callbacks), no audio |
| SDL3 | Window, input, gamepads, audio | Much larger; would overlap with our audio and renderer choices |
| SFML | Simple C++ API | Brings its own graphics and audio we do not use |

**Choice: GLFW**, as a platform backend (`platform-glfw`). The platform is a contract too: the example
`examples/graphic/platform/sdl/SdlVulkanWindow` runs the same Vulkan renderer on an SDL3 window, which shows that the
choice can be undone.

## Scripting: Luau

The subject suggests Lua or Python. Gameplay must be written without C++ (C3), hot-reloaded, and a broken script must
never take the game down (rule 9).

| Option | Speed | Types | Sandbox | Embedding | Against |
|---|---|---|---|---|---|
| Lua 5.4 | Good | None | By hand | Tiny C API | No types; sandboxing is manual |
| LuaJIT | Excellent | None | By hand | Lua 5.1 API | Stuck at Lua 5.1; JIT forbidden on some platforms |
| Python (CPython) | Slow | Hints only | Hard | Heavy, a GIL | Large runtime, hard to sandbox, slow for per-frame calls |
| JavaScript (QuickJS, V8) | Good to excellent | With TypeScript | Good | QuickJS small, V8 huge | Less used in game scripting |
| C# (Mono, CoreCLR) | Excellent | Static | Weak | Heavy | A second runtime and garbage collector |
| **Luau** | Faster than Lua 5.1, per its authors (an interpreter tuned by Roblox) | **Gradual types**, checked by a type checker | **Built in** (`luaL_sandbox`, frozen tables) | Lua C API, C++ sources | Smaller ecosystem than Lua; no JIT on every platform |

**Choice: Luau.** Its type checker lets us generate `.d.luau` declarations of the engine's API for editors, its sandbox
isolates each script file ([`luau/sandbox.md`](../luau/sandbox.md)), and it is used at scale (Roblox) by people who
are not engine programmers, which is exactly our game developers. The bridge design is in
[`ecs/features/19-luau-bridge.md`](../ecs/features/19-luau-bridge.md).

## Physics: Jolt

Full comparison, scores and reasons: [`../benchmarks/physics.md`](../benchmarks/physics.md).

| Library | Score /45 | Why not / why |
|---|---|---|
| **Jolt Physics** | **41** | 2D today (`Plane2D`) and 3D later with one library; single-threaded job system fits one thread per room; character controller for prediction |
| Box2D v3 → Box3D | 32 | Best 2D, but 3D is an alpha and a second library |
| PhysX 5 | 29 | Heavy, no package |
| ReactPhysics3D | 26 | Few releases |
| Bullet 3 | 23 | Maintenance mode, pointer-heavy API |

## Audio: miniaudio

Full comparison: [`../benchmarks/audio.md`](../benchmarks/audio.md).

| Library | Why not / why |
|---|---|
| **miniaudio** | Playback, decoding and mixing with buses in one dependency-free library; public domain / MIT-0; no global context (C6) |
| OpenAL Soft | LGPL, global context, 3D we do not need yet |
| SDL3 audio / SFML audio | Bring a windowing layer we already have |
| FMOD / Wwise | Proprietary (C8) |

## Networking library: Asio, our own protocol

| Option | For | Against |
|---|---|---|
| BSD sockets / Winsock | No dependency | Two APIs to wrap, error handling differs per OS |
| **Asio (standalone)** | One portable socket API, header-only, the base of the C++ networking proposal | Callback style we do not use (we poll) |
| ENet | Reliability over UDP ready-made | Its own connection model and channels; no snapshot or manifest; one more thing to hide |
| yojimbo / GameNetworkingSockets | Complete | Large, opinionated, encryption and relay we do not need |
| TCP for reliable messages | Simple | Head-of-line blocking, two sockets to keep in sync; the subject wants UDP in game |

**Choice: Asio for the socket only**, with our own reliability layer (sequence numbers, ack bits, a reliable ordered
channel) on top, modeled on Glenn Fiedler's articles and the netcode standard. Writing it ourselves let us shape it for
snapshots: see [Algorithms](algorithms.md#replication) and the [protocol](../network/protocol.md).

## User interface and editor

| Need | Choice | Alternatives considered |
|---|---|---|
| In-game UI (menus, HUD) | **Our ECS-native UI + Yoga** (flexbox layout). A node is an entity, so scripts, capsules and the editor handle UI like anything else ([`Ui.md`](../Ui.md)) | Dear ImGui (a debug UI, not a game UI), RmlUi (HTML/CSS, its own DOM next to the ECS), Noesis (proprietary) |
| Editor panels | **Dear ImGui** (docking branch), private to the `editor` module | Qt (huge, LGPL), our own UI (not mature enough for tools) |
| Text | Our own signed-distance-field text ([`Text.md`](../Text.md)) | FreeType at run time (a dependency per platform), bitmap fonts (blurry when scaled) |

## Other libraries

| Need | Library | Why |
|---|---|---|
| Math | GLM | The de-facto standard, matches GLSL |
| Images | stb_image | One header, no dependency |
| JSON | nlohmann/json | Readable API; JSON is not on a hot path |
| HTTP (for games that call web APIs) | libcurl + OpenSSL | The reference client; built static, never the system's ([`Http.md`](../Http.md)) |
| Plugin signatures | libsodium | Ed25519 done right, small API ([`Plugins.md`](../Plugins.md#signatures)) |
| `expected` before C++23 libraries have it | tl::expected | Drop-in for `std::expected` |
| Tests | GoogleTest | Standard, xmake integration, one binary per module |
| Style and lint | clang-format, clang-tidy | Run in CI on the changed files |
| Documentation | Markdown + Zensical (Material theme) | Docs live next to the code, are reviewed in pull requests, and are published as a searchable site |
