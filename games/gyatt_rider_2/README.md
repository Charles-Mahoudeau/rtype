# GyattRider 2

A free-for-all for up to 4 riders on 8 vertical roads, seen from above, written in Luau only. The roads scroll down
fast while the riders race up; slow traffic comes from behind and overtakes them. The riders shoot beams at each other
and at the cyclists. Whoever has the best score after 2 minutes wins.

## Run it

Build the engine (`xmake build -y`), then from the repository root:

```sh
./r-type_server games/gyatt_rider_2
./r-type_client games/gyatt_rider_2     # once per player
```

The game folder comes first. `--port <port>` on both sides and `--host <address>` on a client play over the network,
and `--fullscreen` on a client fills the screen (the window can be resized otherwise). The first 4 clients ride, the
next ones watch; a match starts once 2 ride. The game is only Luau and assets: after changing it, restart the server
and the clients, no build needed.

## Controls

| Key | Action |
|---|---|
| Z Q S D (W A S D on a QWERTY keyboard) | Move in every direction |
| Arrows | Fire a beam up, down, left or right |
| F3 | Show the hitboxes |
| Escape, or the window's close button | Quit |

## Rules

- **The match** waits for 2 riders, races for 2 minutes, shows its results for 10 seconds, then waits for the next one.
  A player who joins during a race rides at once, from 0.
- **The traffic** (cars, electric cars, vans, trucks, buses, limousines, cyclists) comes from behind, each kind at its
  own speed (the cyclists fastest), and slows down behind slower traffic. A vehicle or a cyclist that runs over a
  rider hits it, again and again while the rider stays inside it.
- **A shot** is an instant beam, every 0.25 seconds while an arrow is held. It stops at the first thing it meets: a
  rider is hit, a cyclist is knocked off the road, a vehicle just stops it.
- **A hit rider** becomes a ghost for 2 seconds: nothing hits it, and it cannot shoot.

| Score | Points |
|---|---|
| Every second of the race | +5 |
| A shot fired (needs that many points to fire) | -20 |
| A shot that reaches a rider or a cyclist | +40 |
| A hit rider | loses a tenth of its points, which its shooter takes (the traffic takes them from nobody) |

A score never goes below 0, and counts only while the match races.

**Boosts.** Half the cyclists carry one, shown by their color; shooting the cyclist gives it:

| Color | Boost |
|---|---|
| Gold | +50 points |
| Cyan | 1.5 times faster for 15 seconds |
| Green | Invincible for 5 seconds (and still able to shoot) |
| Red | Sniper shots for 5 seconds: the beam goes through everything to the view's edge |

## On screen

- Each rider is in its player's color, the one of its score; the player's own rider pedals faster than the others.
  The cyclists are white.
- The scores along the top, the time left and the race's gauge in the strip on the left (the covered part in orange,
  the cyclists as small orange cyclists riding), the player's own score in the bottom right corner.
- The results in the middle when a match is over: the winner, every rider's place and points.
- "Player 2 left" under the scores, in that player's color, when a player leaves.
- An impact at the start of each beam, where it stops and on each death; the view shakes when the player is hit.
- Each boost's effect on its rider: rising arrows for the speed, a burst of twinkles for the points, a bubble shading
  white and green for the invincibility, a pulsing red scope for the sniper shots.
- The music, and a sound for each shot, each death and the player's own kills.

## Layout

```
project.luau            settings: network, plugins, window (1600x900, 16:9), audio, physics
scripts/shared/         the rules both sides share (`rules.luau`: the Game table, the replicated components, the
                        inputs), required by the entry `shared/main`
scripts/server/         the gameplay, `main.luau` its entry: the players and their riders, the traffic, the shots,
                        the hits, the shields, the boosts, the match and the score
scripts/client/         the presentation, `main.luau` its entry: the scene, the looks of what moves, the beams, the
                        HUD, the gauge, the results, the sounds, the effects and the controls
assets/actors/          the rider and each kind of traffic (hitbox, look, speed), spawned by the server
assets/scenes/          the places of the camera and of the strip on the left, loaded by the clients
assets/                 the art, the roads map, the font, the sounds and the particles
tools/make_placeholders.py   draws the placeholder art
```

Every size is counted in tiles of 64 pixels: a road is 3 tiles wide, a rider and a cyclist 1x2, a car 2x3, a limousine
2x7. A vehicle drives in the middle of its road, a cyclist on any of its 3 tiles. Each hitbox is 16 pixels smaller on
each side than its image.

## Assets

Real art replaces a file with the same name and the same size in tiles, facing up. The riders and the cyclists share
`cyclists/cyclist1.png`, `2` and `3`, the frames of their riding animation (1 2 3 2 1...), which each client
tints: draw them white or light grey, then run `python3 tools/pack_cyclist.py`, which packs them into
`cyclists/cyclist_sheet.png`. `beam.png` is drawn for a shot to the
right, and the game turns it. `tools/make_placeholders.py` redraws every placeholder and overwrites what is there: do
not run it over real art.

## Network

The server owns the gameplay and the clients draw it. A snapshot holds about 9 entities, sent whole, so the game keeps
its replicated entities under that (`Game.maxReplicated`): a shot is the shooter's `game.Shot`, not an entity, and a
vehicle sends its position every 0.3 seconds and its speed when it changes, the clients moving it in between. The
clients give each vehicle's body more room in the physics (`physics.maxBodies`), as they never step their world
(engine issue).
