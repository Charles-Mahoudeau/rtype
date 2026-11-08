# GyattRider 2

A free-for-all for up to 4 riders on 8 vertical roads, seen from above, written in Luau only. The roads scroll down
fast while the riders race up; slow traffic comes from behind and overtakes them. Whoever has the best score after 4
minutes wins.

## Run it

Build the engine (`xmake build -y`), then from the repository root:

```sh
./r-type_server --game games/gyatt_rider_2/game
./r-type_client --game games/gyatt_rider_2/game     # once per player
```

`--port <port>` on both sides and `--host <address>` on a client play over the network. The first 4 clients ride, the
next ones watch.

## Controls

| Key | Action |
|---|---|
| Z Q S D (W A S D on a QWERTY keyboard) | Move in every direction |
| F3 | Show the colliders |

## What is in it so far

- The 8 roads scrolling, and the strip on the left for the timer and the gauge.
- A rider for each of the first 4 players, in their slot's color (blue, red, green, purple), moved by the server and
  outlined in white on its own player's screen.

Planned: the traffic (cars, electric cars, vans, trucks, buses, limousines, cyclists), shooting with the arrow keys,
the score and the 4-minute match, the cyclists' boosts, the HUD, then the art, the sounds and the effects.

## Layout

```
game/
  project.luau          settings: network, plugins, window (1088x640)
  scripts/shared/       the rules both sides share (`rules.luau`: the Game table, the replicated components, the
                        inputs), required by the entry `shared/main`
  scripts/server/       gameplay: the players' riders and their moves; `main.luau` is the server's entry
  scripts/client/       presentation: the assets, the roads, the riders' look; `main.luau` is the client's entry
  assets/actors/        the rider (body and collision layers), spawned by the server for each player
  assets/scenes/        the places of the camera and of the strip on the left, loaded by the clients
  assets/               also the placeholder art and the roads map
tools/make_placeholders.py   draws the placeholder art
```

Every size is counted in tiles of 64 pixels: a road is 2 tiles wide, a rider 1x2, a car 2x3, a limousine 2x7.

## Assets

The art in `game/assets/` is flat placeholders drawn by `python3 tools/make_placeholders.py` (Pillow), which
overwrites them. Real art replaces a file with the same name and the same size in tiles, facing up.
