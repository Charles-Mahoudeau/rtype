# examples/games — game work (Luau only)

Read the root `AGENTS.md` first. This file is for **making or changing a game**: the Luau scripts, `project.luau` and
the assets of a `game/` folder. It is not for engine work (that is `src/`).

## What you do and do not do

- You write Luau and `project.luau`, and add assets. A game is the folder `game/` with `project.luau`, `scripts/` and
  `assets/`, run by the generic executables: `./r-type_server --game <dir>` and `./r-type_client --game <dir>`.
- You never read or change C++ (`src/`, a game's own `src/` or `main`). The C++ parts of `rtype_duel` and of
  `ethan_game(bestoftheworld)` are a temporary step, not a model. Do not copy them.
- Learn the API from the game developer manual in `manual/`. Until it covers a topic, `docs/scripting/plan.md` and
  `docs/engine/design.md` hold the details, and `docs/luau/type-definitions.md` the Luau types. Never guess an API: if
  neither says it exists, it does not.
- Choose plugins, window, renderer, audio, network and physics in `project.luau`, never by changing code.
- If the engine lacks something the game needs, do not work around it: file an engine issue (title
  `[Area] <summary>`, say which script wants what, with the Luau you would write) and tell the user. Use the
  `plan-issues` skill if several tasks are needed.

## How

- Use the `luau-gameplay` skill for the folder layout, components, systems, calls and setup forms.
- Model your game on `examples/games/rtype_duel/game/` (scripts split by side: `server/`, `client/`, shared files at the
  root).
- Run it to see it work: build with `xmake build -y`, then run the server and the client (commands in the root file).
- Keep every piece of data on the side it belongs to: the server owns gameplay, the client draws and sends inputs.

## Done when

The game runs with the generic executables, a server and a client connect, and the manual page of any feature you rely
on matches what you used. If you found a manual page that is wrong or missing, fix it in `manual/` in the same change.
