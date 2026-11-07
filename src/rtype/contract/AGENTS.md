# contract — what backends fulfil and plugins use

One shared library per interface (`contract-platform`, `contract-renderer`, `contract-audiodevice`): the interface, its
registry (implementations by name), and the vocabulary they speak.

## Boundaries

- A contract depends on `core` and glm only (the renderer contract also on the platform contract). Never on the ECS or a
  plugin.
- No ECS type in a contract. The ECS declaration of a contract type (its reflection, its component form) lives in the
  plugin that uses it.
- The renderer contract keeps its themes in the flat namespace; the others mirror their folders.

## Invariants

- A contract describes what the window, the GPU or the sound card must understand, never what a game writes.
- Adding a call to an interface means every implementation gets it, in the same change.
- Types that cross a contract are plain data: cheap to copy, holding no engine state.

## Tests and docs

- No test folder of its own: contract types are exercised by `tests/plugins/` and `tests/vulkan/`.
- Docs: `docs/Backend.md`, `docs/Plugins.md`; audio in `docs/audio/design.md`.
