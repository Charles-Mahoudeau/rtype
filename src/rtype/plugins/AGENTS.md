# plugins — the engine features

Each engine feature (transform, input, graphics, sprite, tilemap, text, shader, particle, shape, audio, physics...) is an
ECS plugin: one shared library `plugin-<name>` in one folder, namespace `rtype::plugins::<name>`.

## Boundaries

- A plugin depends on `core`, `ecs`, glm, the contracts and the plugins it requires, plus its own private packages
  (physics engine, JSON, shader compiler, image decoding). Never on a `platform-*`, `render-*`, `audio-*`, `luau`,
  `scripting`, `engine` or `replication` target.
- The graphics plugin starts the backend from a registry the backend libraries fill, so no plugin depends on a backend.
- The plugin graph has no cycle. The graphics plugin knows no plugin that draws; each drawing plugin is ordered against
  it only. A resource several plugins fill is appended to, not owned by one.

## Invariants

- A plugin owns its components, resources and events, and declares which side each lives on.
- A plugin names what it requires; it never adds another plugin. The engine adds the ones `project.luau` lists, and those
  they require come with them.
- A plugin is a library with one `RTYPE_PLUGIN_LIBRARY` entry point registering it by name (`getName()`, `rtype.<Name>`
  for the engine's); the executables link none.
- Drawing runs on the client only; a server adds the same plugins without the draw systems.
- The setup forms a plugin gives scripts (`engine.assets.texture`...) are a `<Name>Forms.luau` file of its own, registered
  from its library: a plugin never links Luau, and a server, which does not add it, still gets the forms.
- What a plugin offers to scripts is an engine call named `rtype.<plugin>.<verb>`, with typed argument and result data,
  registered from the plugin's build step.
- A feature is not done until a script can use it: engine call, manual page, script test.
- A private package never appears in a public header.

## Tests and docs

- Tests: `tests/plugins/` (`plugins-tests`), flat, one file per concern.
- Docs: `docs/Plugins.md` (the graph), one page per feature (`docs/Sprites.md`, `Tilemap.md`, `Text.md`, `Shaders.md`,
  `Input.md`, `docs/physics/design.md`, `docs/audio/design.md`, `docs/particles/design.md`), `docs/Modules.md`.
- Skills: `add-plugin`, `add-component`, `add-engine-call`.
