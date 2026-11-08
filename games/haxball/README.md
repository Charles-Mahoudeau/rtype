# Haxball

Top-down football for up to 4 against 4, written in Luau only. Players are discs that push the ball with their body
and kick it with one key. A match lasts 3 minutes or ends when a team scores 5 goals; a draw goes to golden goal.

It shows the engine's physics (every push, bounce and goal), its particles (kicks, the ball's trail, dust, confetti),
its networking (an authoritative server, inputs, replicated state and the game's own packets), its UI (the title menu
and the HUD) and its audio.

## Run it

Build the engine (`xmake build -y`), then from the repository root:

```sh
./aylo-runtime --server games/haxball
./aylo-runtime games/haxball     # once per player
```

`--port <port>` on both sides and `--host <address>` on a client play over the network (`4242` and `127.0.0.1` by
default). `./r-type_server games/haxball` and `./r-type_client games/haxball` do the same.

The title menu asks for a name and a team: *Red* and *Blue* show how many players they have (4 at most), *Spectate*
watches. The match starts once each team has a player; before that, everyone warms up and nothing is counted.

## Controls

| Key | Action |
|---|---|
| W A S D or the arrows (Z Q S D on an AZERTY keyboard) | Move |
| Space or X | Kick: held, you slow down and kick the ball as soon as it is in reach |
| Escape | Open or close the menu, to change team or name, or quit |
| F3 | Show the colliders |

## Rules

- After a goal, everyone goes back to their half and the team that conceded kicks off, after a 3-2-1 countdown during
  which nobody moves.
- A goal scored into your own net counts for the other team, as an own goal.
- When time runs out on a draw, the match goes on in golden goal: the next goal wins.
- The result stays on screen for 6 seconds, then a new match starts with the same teams.

Every number (sizes, speeds, damping, kick strength, durations) is in `scripts/shared/rules.luau`.

## Layout

```
project.luau            settings: network, plugins, window (1280x720)
scripts/shared/         what both sides share: rules.luau (the Game table, the replicated components, the inputs),
                        packets.luau (game.Join, game.Kicked, game.Bounce, game.Goal); the entry is main.luau
scripts/server/         gameplay: the pitch's walls, posts and goal sensors, the players, the ball, the match
scripts/client/         presentation: the scene, the sprites, the menu, the HUD, the particles and the sounds
assets/                 sprites/, particles/, sounds/, fonts/
tools/                  the scripts that make the art and the sounds
PLAN.md                 how the game was planned
```

Only the server steps the physics. The clients draw the replicated positions, and the server sends packets for what
they cannot see (a kick, a bounce, a goal), on which they spawn particles and play sounds.

## Assets

The art and the sounds are generated, so the game owns them. From the repository root:

```sh
python3 games/haxball/tools/make_art.py      # needs Pillow: the pitch, the players, the ball, the particle textures
python3 games/haxball/tools/make_sounds.py   # needs numpy: kicks, bounces, whistles, beeps, the crowd, the click, the music loop
```

Both overwrite their files in `assets/` and give the same files on every run. The sizes in `make_art.py` repeat
`rules.luau`: change both together.

The font is Orbitron, by The Orbitron Project Authors, under the SIL Open Font License 1.1 (`assets/fonts/OFL.txt`).
