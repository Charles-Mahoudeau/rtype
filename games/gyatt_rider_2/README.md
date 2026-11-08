# GyattRider 2

A free-for-all for up to 4 riders on 8 vertical roads, seen from above, written in Luau only. The roads scroll down
fast while the riders race up; slow traffic comes from behind and overtakes them. Whoever has the best score after 4
minutes wins.

## Run it

Build the engine (`xmake build -y`), then from the repository root:

```sh
./r-type_server games/gyatt_rider_2
./r-type_client games/gyatt_rider_2     # once per player
```

`--port <port>` on both sides and `--host <address>` on a client play over the network, and `--fullscreen` on a
client fills the screen (the window can be resized otherwise). The first 4 clients ride, the next ones watch; a match
starts once 2 ride.

## Controls

| Key | Action |
|---|---|
| Z Q S D (W A S D on a QWERTY keyboard) | Move in every direction |
| Arrows | Fire a beam up, down, left or right |
| F3 | Show the colliders |
| Escape | Quit |

## What is in it so far

- The 8 roads scrolling, and the strip on the left for the timer and the gauge.
- A rider for each of the first 4 players, blue on its own screen and red on the others', moved by the server and
  outlined in white on its own player's screen.
- The traffic: cars, electric cars, vans, trucks, buses, limousines and cyclists come from behind, each kind at its
  speed, and slow down behind slower traffic. A vehicle that runs over a rider hits it.
- Instant shots: a beam from the rider to the first thing it meets. A rider it reaches is hit; a cyclist it reaches is
  knocked off the road; a vehicle stops it.
- The 4-minute match: it waits for 2 riders, races, shows its results, then waits for the next one.

Planned: the score, the cyclists' boosts, the HUD, then the art, the sounds and the effects.

## Layout

```
project.luau            settings: network, plugins, window (1600x900, 16:9)
scripts/shared/         the rules both sides share (`rules.luau`: the Game table, the replicated components, the
                        inputs), required by the entry `shared/main`
scripts/server/         gameplay: the riders, the traffic, the shots, the hits, the match; `main.luau` is the
                        server's entry
scripts/client/         presentation: the assets, the roads, the looks of what moves, the beams, the controls;
                        `main.luau` is the client's entry
assets/actors/          the rider and each kind of traffic (body, look, speed), spawned by the server
assets/scenes/          the places of the camera and of the strip on the left, loaded by the clients
assets/                 also the placeholder art and the roads map
tools/make_placeholders.py   draws the placeholder art
```

Every size is counted in tiles of 64 pixels: a road is 3 tiles wide, a rider 1x2, a cyclist 1x2, a car 2x3, a
limousine 2x7. A vehicle drives in the middle of its road, a cyclist on any of its 3 tiles.

## Assets

The art in `assets/` is flat placeholders drawn by `python3 tools/make_placeholders.py` (Pillow), which
overwrites them. Real art replaces a file with the same name and the same size in tiles, facing up.
