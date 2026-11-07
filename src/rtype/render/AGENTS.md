# render — renderer backends

One folder and one shared library per graphics API (`render-<api>`), implementing the renderer contract. Today: Vulkan.
The platforms (`src/rtype/platform/`) and audio backends (`src/rtype/audio/`) follow the same boundaries.

## Boundaries

- A backend links its contract only: no ECS, no plugin. Backends never depend on each other. What a renderer needs from
  a window that is specific to its API comes through the matching `interop-*` target, which both sides depend on.
- Vulkan and its helper libraries are private to this folder; the same goes for the window library in the platform
  backend and the audio library in the audio backend.
- A backend is a library the engine loads: it exports one `RTYPE_LIBRARY` entry point (`Library.cpp`) that fills the
  registry of its contract by name. The executable links it not at all (`docs/Plugins.md#loading`).

## Invariants

- GPU objects are owned by RAII wrappers released in reverse order of creation; the device idles before teardown.
- A driver failure is an exception naming the result, never a silent return. A swapchain that goes out of date is
  rebuilt, not an error.
- Shaders of the backend live in its `shaders/` folder and are compiled by its build rule; a game's own shaders arrive
  as compiled data through the contract.

## Tests and docs

- Tests: `tests/vulkan/` (`vulkan-tests`).
- Docs: `docs/Vulkan.md`, `docs/Backend.md`, `docs/Shaders.md`, `docs/gpu-debugging.md`.
