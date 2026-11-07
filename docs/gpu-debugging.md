# GPU debugging

How to read validation messages and capture a frame of the Vulkan renderer.

## Turning it on

Names and labels need `VK_EXT_debug_utils`, which the renderer enables with `vulkan.debugging`:

```cpp
settings.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
settings.set("vulkan.debugging", true);
settings.set("vulkan.minSeverity", "warning");
```

The same settings in a game's `project.luau`:

```lua
engine.settings.vulkan.layers = { "VK_LAYER_KHRONOS_validation" }
engine.settings.vulkan.debugging = true
```

Without `vulkan.debugging`, naming and labelling do nothing and cost nothing on the GPU.

On macOS, if the loader does not find the Homebrew layers, point it at them:

```sh
VK_ADD_LAYER_PATH=/opt/homebrew/share/vulkan/explicit_layer.d xmake run shader
```

A layer the machine does not have is not an error: the renderer writes `[vulkan] warning: the layer ... is not installed:
running without it` on the error output and starts without it. So does a layer the loader lists but cannot open (its
manifest is there, its library is not: `[vulkan] warning: a layer could not be loaded`), and synchronization validation or
best practices the validation layer does not offer. A project can ask for the validation layer in debug builds without
breaking the machines that do not have it.

## Names

Every object the renderer creates has a name, shown in validation messages and captures instead of a hexadecimal
handle: `Sprite pipeline`, `Frame 1 uniforms`, `Texture 12`, `Swapchain image 2`. The pipelines of a game shader are
named after it: `flash particle pipeline`. The instance, the surface, the physical device and the queues have none.

To name a new object, call `core::Device::setName()` right after creating it:

```cpp
device.setName(*buffer.getBuffer(), "Sprite instances");
```

Code that only holds an allocator gets the device from `Allocator::getDevice()`.

`core::DebugMessenger` prints them under the message, with the labels being recorded:

```text
[Vulkan67 Error] { Validation } vkCmdSetViewport(): pViewports[0].width (0.000000) is not greater than zero.
The Vulkan spec states: width must be greater than 0.0 (...)
  In: Sprites
  CommandBuffer: Frame 0 command buffer
```

## Labels

A frame is recorded as these labelled groups, in order: `Particle simulation`, then `Sprites` and `Particles`, once per
run of them, as they are drawn by layer, and `Shapes`. To label a new pass:

```cpp
const command::CommandLabel label{device, commandBuffer, "My pass"};
myPass.record(commandBuffer);
```

## Capturing a frame

| Platform | Tool | How |
|---|---|---|
| Linux, Windows | [RenderDoc](https://renderdoc.org/) | Launch the executable from RenderDoc, then press F12 or Print Screen in the game. |
| Linux, Windows (NVIDIA) | [Nsight Graphics](https://developer.nvidia.com/nsight-graphics) | Start a Frame Debugger activity on the executable. |
| macOS | Xcode (Metal capture) | See below. RenderDoc does not run on macOS. |

Set the working directory to the project root, as `xmake run` does: the assets are loaded relative to it.

### Xcode and MoltenVK

Vulkan runs on Metal through MoltenVK, so a capture shows Metal commands. Ask MoltenVK to capture, then open the file:

```sh
MVK_CONFIG_AUTO_GPU_CAPTURE_SCOPE=2 MVK_CONFIG_AUTO_GPU_CAPTURE_OUTPUT_FILE=/tmp/frame.gputrace \
    METAL_CAPTURE_ENABLED=1 build/macosx/arm64/release/shader
open /tmp/frame.gputrace
```

`MVK_CONFIG_AUTO_GPU_CAPTURE_SCOPE=2` captures the first frame. The labels appear as debug groups.
