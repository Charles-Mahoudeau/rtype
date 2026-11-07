---
name: add-plugin
description: Add a new engine plugin (an ECS plugin library under src/rtype/plugins) to the R-Type engine, from the build file to Luau reachability. Use when asked to "add a plugin", "create a new engine feature", or when a feature needs its own components, resources, events and systems.
---

# Add an engine plugin

A plugin is one engine feature as an ECS plugin: one shared library `plugin-<name>` in `src/rtype/plugins/<name>/`.
Read `src/rtype/plugins/AGENTS.md`, `docs/Plugins.md` (the graph and the rules) and `docs/CodeStyle.md` first. Look at
a small existing plugin of the same shape (for instance the shape or sprite plugin) and copy its structure. For an
earlier example of splitting plugins into libraries, see pull request #414.

## Checklist

1. **Decide the boundaries.** What does it own (components, resources, events, systems)? Which plugins does it require?
   It may depend on `core`, `ecs`, glm, the contracts and the plugins it requires, plus private third-party packages.
   Never on a `platform-*`, `render-*`, `audio-*` or `luau` target, and no cycle in the plugin graph. Which side runs
   what: drawing is client-only; gameplay data says whether it is server, client or both.
2. **Create the folder** `src/rtype/plugins/<name>/`, namespace `rtype::plugins::<name>`, one feature folder inside only
   when files go in it, an export header for `RTYPE_PLUGIN_<NAME>_API`, exceptions in `exceptions/`.
3. **Declare the target** in `src/rtype/plugins/xmake.lua` with the `plugin("<name>", {<deps>})` helper: its sources and
   headers by glob, and its private packages with `add_packages`. The helper applies the `rtype.plugin_library` rule,
   which puts the library where the engine loads it. A new third-party package is added with
   `add_requires` (in that file, or the root one if several modules use it) and stays private: no public header
   includes it.
4. **Write the plugin class.** It declares its requirements in the dependencies it reports and registers its data in its
   build step. It never adds another plugin; the program adds them, requirements first. Follow the rule of five and the
   rest of the code style.
5. **Declare the data** with the ECS macros so scripts, serialization and replication see it: see the `add-component`
   skill. Put the name in the `rtype.` namespace for engine data.
6. **Settings.** If it reads a section of `project.luau`, read it through the shared settings, refuse unknown keys, and
   check the values before the app is built.
7. **Make it loadable and reachable from Luau.** Give the library its entry point, a `Library.cpp` with one
   `RTYPE_PLUGIN_LIBRARY` that registers the plugin under its `getName()` (`rtype.<Name>`), as the other plugins do
   (`docs/Plugins.md`, *Entry points*). A game then lists that name under `plugins` in `project.luau`; no executable
   or engine file changes, and the plugins it requires come with it. Say which side it runs on. Register its engine
   calls from the build step (skill `add-engine-call`).
8. **Tests** in `tests/plugins/`: one file per concern; add the plugin to the dependencies of `plugins-tests` in
   `tests/plugins/xmake.lua`. Cover the success and failure paths, and the plugin's requirement check.
9. **Script test** in `tests/engine/` for what scripts can do with it, next to `ScriptEngineCallsTests`.
10. **Docs.** Add the plugin to the tables of `docs/Plugins.md`; give it a design page in `docs/` (or a section of an
    existing one) with a "From scripts" section; add its description to `docs/Modules.md`; add its page in `manual/` (invoke the `write-manual` skill)
    (see `manual/mkdocs.yml` for the nav) for what a game developer uses, with no C++ in it.
11. **Verify** with the `verify` skill, then commit with `commit-split` (build file and plugin first, then data, then
    calls, then tests, then docs) and open the pull request with `open-pr`.

## Done means

A script can use the feature, the manual explains it, a script test covers it, `docs/Plugins.md` lists it, and the
dependency rules still hold.
