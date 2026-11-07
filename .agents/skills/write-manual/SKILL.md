---
name: write-manual
description: Write or update the game developer manual (manual/pages) for the Luau scripting API and the engine features a script can use - calls, components, events, settings, setup forms, plugins - in a friendly, complete, example-driven style. Use whenever you add or change anything a script can see (engine call, component, event, setting, setup form, plugin), when a manual page is missing, wrong or thin, or when asked to "document", "write the manual", "update the docs for scripts".
---

# Write the game developer manual

The manual (`manual/pages/`, built with Zensical, published to GitHub Pages) is the only API reference a game developer
has: `examples/games/AGENTS.md` forbids them to read C++. **If a script can see it, the manual explains it, in the same
change.** A feature with no manual page is not done (root `AGENTS.md`, rule 2).

## When to run this skill

Any change that alters what a script can observe:

- an engine call added, renamed, or given a new argument, result, default or error (`rtype.<plugin>.<verb>`);
- a component, resource or event of a plugin: a field, a type, a default, the side it lives on;
- a setup form (`engine.assets`, `engine.input`...) or an `engine.*` helper;
- a `project.luau` setting or a plugin name;
- a system phase, ordering name (`rtype.Physics.SyncBodies`...) or a rule of the scripting bridge;
- a new plugin; networking or replication behavior a game sees.

Pure internals (a refactor, a private class) need no manual change, but say so in the pull request.

## Before writing

1. **Read the code or the design doc for the truth**, not your memory: the plugin's `*Calls.hpp` (arguments, defaults,
   result), its components, `docs/<feature>.md`, and the script test in `tests/engine/`. The manual must match them
   exactly: names, field types, defaults, units, which side.
2. **Read the page you will change** and its neighbours (`manual/pages/physics.md`, `engine-calls.md`, `components.md` are
   the model) so the voice, headings and tables stay consistent.
3. **Find the home**: an existing page for the feature, or a new page registered in `manual/mkdocs.yml` (`nav`) and linked
   from `index.md` and from the pages that mention it.

## How to write it

**Audience**: someone who knows programming and a little Luau, never saw this engine, and will copy your example. Write
Luau only; never mention C++, a header or a class of `src/`.

**Tone**: friendly and direct, second person ("you add the plugin in `project.luau`"), present tense, short sentences.
Say what a thing is for before how it works. No marketing, no filler, no "simply" or "just".

**Page shape** (drop a part only if it truly does not apply):

1. A one-paragraph intro: what the feature gives a game, and the smallest `project.luau` change to turn it on.
2. A **minimal working example** early: a few lines the reader can paste and see something.
3. **Reference tables**: components (name, fields with types and defaults, side), events, calls (name, arguments with
   defaults, result), settings. Every row is complete: no "etc.", no "see source".
4. **Recipes**: one example per common task ("make a bullet", "play a sound on hit"), each runnable as written, with the
   `phase`/`side`/`calls`/`query` it needs, because a system that forgets to declare a call fails.
5. **Pitfalls and errors**: the exact message a script sees and what causes it; which side has the feature; ordering
   rules; what the server owns and a client cannot write.
6. **See also**: links to the related pages.

**Examples must be true.** Every snippet is Luau that runs: names, argument tables and phases are those of the
engine. Prefer taking them from, or checking them against, `examples/games/rtype_duel/game/` and the script tests. If you
can, run the snippet (a script test, or the duel game) before you publish it. An example that was not checked is worse
than none.

**Consistency rules**:

- Units and axes are stated once and the same everywhere (world units, Y down, seconds).
- Say the side of every piece of data: server, client, both. A client never writes replicated data.
- Use the names the engine uses (`rtype.Collider`, `rtype.physics.raycast`), in code font; link a first mention to its
  page.
- Admonitions (`!!! note`, `!!! warning`) for the one thing the reader must not miss; not more than one or two a page.
- Tables for facts, prose for reasons, code for how.
- Links are relative (`setup-forms.md`) and must resolve.

## After writing

1. Build the site and read the page as a newcomer would: `uvx zensical build -f manual/mkdocs.yml` must succeed with no
   broken link or nav warning.
2. Re-read each snippet against the code once more; check every name you wrote appears in the engine (`grep` it).
3. Update the design doc of the module (`docs/...`) when it holds the same facts, so they do not drift.
4. Put the manual in its own `docs(manual): ...` commit (`commit-split` skill) and say in the pull request which pages
   changed. If you could not run an example, say so.

## Done when

A newcomer can read only the manual, write a working script for the feature, and meet no name, default or behavior that
the page did not tell them.
