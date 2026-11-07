---
name: add-engine-call
description: Add an engine call, a named function a plugin gives to Luau scripts (rtype.<plugin>.<verb>), and make the feature reachable from scripts with a manual page and a script test. Use when asked to "expose X to Luau", "add an engine call", "let scripts do X", "add a setup form", or when a plugin feature needs to be usable by a game.
---

# Add an engine call

An engine call is a named function registered by a plugin that scripts invoke by name with a table of arguments
(`engine.call("rtype.audio.loadSound", {...})` while loading, `ctx.call(...)` in a system that declared it). It is how a
game reaches what plain data cannot do: load an asset, bind an input, play a sound, query physics. Read
`docs/ecs/features/19-luau-bridge.md` (*Engine calls*, *Setup forms*), `docs/ecs/features/12-systems.md` (engine calls),
`docs/scripting/plan.md`, and one existing example: the audio, input or sprite plugin's calls file (pull requests
#415 to #418 added the mechanism, the plugins' calls and the setup forms).

**The rule: a feature is not done until a script can use it, the manual explains it and a script test covers it.**

## Checklist

1. **Name it** `rtype.<plugin>.<verb>` (camelCase verb: `rtype.audio.loadSound`). One call does one thing.
2. **Define the data.** An argument struct and, if the call returns something, a result struct, each declared with the
   call-data macro and a stable `rtype.<plugin>.<Name>` name, in the plugin's calls header (`<Name>Calls.hpp`). Missing
   argument fields keep their defaults, so give every field a sensible default. Use asset-id fields for assets (a script
   passes a path), strings of the ECS (`ecs::String`, 255 bytes at most) for short text, `std::string` for a text that
   may be longer (a prompt, a request body, a reply) and `std::vector` for a list, and glm vectors for positions. An optional value is a documented
   sentinel (NaN for "not given"); say it in the field's comment.
3. **Write the function.** Its parameters are system parameters (resources, queries, event writers), so the call
   declares its access and the scheduler orders it. Register it from a `add<Name>Calls()` function in the plugin's
   `<Name>Calls.cpp`, called from the plugin's build step. Failures raise the call exception with a message naming the
   call; never crash. A call on a side that does not have the plugin is absent there and the scripts and setup forms
   skip it (a server has no textures).
4. **Setup forms.** If it is part of declaring a game's setup (assets, input), add or extend a form of `engine.assets`
   or `engine.input` in the plugin's own `<Name>Forms.luau` (listed in `src/rtype/plugins/xmake.lua`, registered from
   its library and build step, written with the `engine.forms` helpers of `scripting/setup/FormsPrelude.luau`) so the
   script writes one readable table; keep forms thin over the call. The form tests `engine.hasCall()`, so a side without
   the plugin skips it.
5. **Tests.**
    - C++: the call itself in `tests/plugins/` (success, bad argument, the side it is absent on).
    - Script test in `tests/engine/` (see `ScriptEngineCallsTests`, `ScriptSetupFormsTests`, `ScriptPhysicsTests`):
      a Luau snippet makes the call on a real app and checks the result and the error messages.
6. **Manual.** Invoke the `write-manual` skill and add the call to its plugin's page in `manual/`: name, arguments with
   types and defaults, result, which side, an example script, errors. Add it to the calls table of `docs/ecs/features/19-luau-bridge.md` and the
   "From scripts" section of the plugin's design doc.
7. `verify`, `commit-split` (data and function, then registration, then tests, then docs), `open-pr`.

## Pitfalls

- A system must list a call in `calls` before using it with `ctx.call`; an unknown call in a system that does not run
  in the app's mode is accepted, in any other it is refused at load. Document which side registers it.
- A name chosen is a public API: scripts depend on it.
- Do not special-case a plugin in `scripting`: the generic call mechanism is the only path.
