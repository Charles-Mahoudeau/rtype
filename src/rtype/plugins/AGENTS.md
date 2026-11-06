# plugins — the engine features

Each engine feature (transform, input, graphics, sprite, tilemap, text, shader, particle, shape, audio, physics...) is an
ECS plugin: one shared library `plugin-<name>` in one folder, namespace `rtype::plugins::<name>`.

## Boundaries

- A plugin depends on `core`, `ecs`, glm, the contracts and the plugins it requires, plus its own private packages
  (physics engine, JSON, shader compiler, image decoding). Never on a `platform-*`, `render-*`, `audio-*`, `luau`,
  `scripting`, `engine` or `replication` target.
- The graphics plugin starts the backend from a registry the executable fills, so no plugin depends on a backend.
- The plugin graph has no cycle. The graphics plugin knows no plugin that draws; each drawing plugin is ordered against
  it only. A resource several plugins fill is appended to, not owned by one.

## Invariants

- A plugin owns its components, resources and events, and declares which side each lives on.
- A plugin names what it requires; it never adds another plugin (the program does, in order).
- Drawing runs on the client only; a server adds the same plugins without the draw systems.
- What a plugin offers to scripts is an engine call named `rtype.<plugin>.<verb>`, with typed argument and result data,
  registered from the plugin's build step.
- A feature is not done until a script can use it: engine call, manual page, script test.
- A private package never appears in a public header.

## Tests and docs

- Tests: `tests/plugins/` (`plugins-tests`), flat, one file per concern.
- Docs: `docs/Plugins.md` (the graph), one page per feature (`docs/Sprites.md`, `Tilemap.md`, `Text.md`, `Shaders.md`,
  `Input.md`, `docs/physics/design.md`, `docs/audio/design.md`, `docs/particles/design.md`), `docs/Modules.md`.
- Skills: `add-plugin`, `add-component`, `add-engine-call`.
