# Third-party software in the runtime

The engine uses the libraries below. Each is under its own license; the license texts are in the projects named here.
This list is made by hand from the `add_requires` of the build files: update it when a dependency is added, removed or
bundled differently.

| Library | Used for | License | Project |
|---|---|---|---|
| Luau | the scripting language | MIT | https://github.com/luau-lang/luau |
| GLFW | windows and input | zlib | https://www.glfw.org |
| Vulkan loader, headers | graphics API | Apache-2.0 | https://github.com/KhronosGroup |
| Vulkan Memory Allocator | GPU memory | MIT | https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator |
| glslang | shader compiler | BSD-3-Clause, Apache-2.0 | https://github.com/KhronosGroup/glslang |
| Dear ImGui | debug interface | MIT | https://github.com/ocornut/imgui |
| GLM | mathematics | MIT | https://github.com/g-truc/glm |
| stb | image decoding | MIT or public domain | https://github.com/nothings/stb |
| Asio | networking | BSL-1.0 | https://think-async.com |
| libsodium | signatures of the game plugins | ISC | https://libsodium.org |
| Jolt Physics | physics | MIT | https://github.com/jrouwe/JoltPhysics |
| nlohmann/json | data files | MIT | https://github.com/nlohmann/json |
| Yoga | user interface layout | MIT | https://github.com/facebook/yoga |
| miniaudio | audio | MIT-0 or public domain | https://miniaud.io |
| tl::expected | results | CC0-1.0 | https://github.com/TartanLlama/expected |
