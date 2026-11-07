# Walkthrough: R-Type Duel

R-Type Duel is a complete two-player game written only in Luau, in `examples/games/rtype_duel/game/`. Two players sit
on either side of an arena. Each moves up and down, clicks to fire lasers, and the first to hit the other wins. This
page reads it file by file.

```
game/
  project.luau            settings
  scripts/
    arena.luau            shared: constants, components, input actions, loads the arena
    server/               gameplay (runs on the server)
      players.luau        gives each client a ship
      obstacles.luau      tells the clients where the obstacles are
      ships.luau          moving and firing
      lasers.luau         flying, hitting, winning
    client/               presentation (runs on the clients)
      assets.luau         loads atlases, sounds, font, map
      scene.luau          camera, background, obstacles' look
      sprites.luau        looks of ships and lasers, explosion
      sounds.luau         music and shots
      hud.luau            names and the winner text
      controls.luau       a local quit key
  assets/                 images, sounds, font, Tiled map
    scenes/arena.scene    the arena: the walls and the rock, as instances of the actors
    actors/               wall, rock, the two ships and the laser: what the server spawns
```

The arena and what the server spawns in it are [capsules](capsules.md): files you can open and change in the
[editor](editor.md) (`./aylo-runtime --editor --game examples/games/rtype_duel/game` opens the arena).

Scripts load in sorted path order on every side, so `arena.luau`, which sorts before the folders, runs first and gives
the other files a shared `Duel` table.

## Run it

```sh
./aylo-runtime --server --game examples/games/rtype_duel/game
./aylo-runtime --game examples/games/rtype_duel/game      # twice
```

Or `xmake f -y --rtype_duel=y` then `xmake run rtype_duel server` and `xmake run rtype_duel client`.

## `project.luau`: the data that picks the engine

```luau
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end

engine.settings.plugins = { "rtype.Physics" }
if engine.isClient then
	engine.settings.plugins = {
		"rtype.Physics", "rtype.Input", "rtype.graphics", "rtype.Shader", "rtype.Sprite", "rtype.SpriteDraw",
		"rtype.Shape", "rtype.Tilemap", "rtype.Text", "rtype.Particle", "rtype.audio", "rtype.PhysicsDebug",
	}
end
engine.settings.physics.gravity = { 0, 0 }
```

The port and host come from the command line. A top-down arena needs the physics with no gravity, and the debug view to
see the colliders (F3). The server lists only the physics; a client lists everything it draws, plays and reads. Clients
also choose `platform = "glfw"`, `renderer = "vulkan"`, a window and an audio backend.
See [Project settings](project-settings.md).

## `arena.luau`: the rules both sides share

It declares the constants (`Duel.shipSpeed`, the collision layers), then the replicated components:

```luau
engine.ecs.component("game.Ship", {
	fields = { position = "vec2", player = "u32" },
	replication = "everyChange",
})
engine.ecs.component("game.MisfireTimer", { fields = { remaining = "f32" }, presence = "serverOnly" })
```

`game.Ship`, `game.Laser`, `game.Obstacle`, `game.Winner` and `game.Misfire` are replicated: the clients see them
appear and move. `game.MisfireTimer` exists on the server only. Then it declares the inputs, once, for both sides:

```luau
engine.input.action("move", "vector2", { bind = { { up = "W", down = "S", left = "A", right = "D" }, ... } })
engine.input.action("shoot", "button", { bind = { "MouseLeft" } })
```

See [Components](components.md) and [Setup forms](setup-forms.md).

## The server

Every server file starts with `if not engine.isServer then return end`, because its systems are ordered against
systems only the server has. `players.luau`, `obstacles.luau` and `ships.luau` also return when `Duel.networked` is
false: in the editor, the server and the client are one process with no network, so no player joins.

`arena.luau` loads the arena on every side in a `Startup` system, with `engine.capsule.load(ctx, Duel.arena)`. The server
spawns its walls and its rock and replicates them; a client spawns no node of it, as they are all the server's. The
scene names no component of a client's plugin: a dedicated server reads it too, and has none of them.

- **`players.luau`** reads `net.ClientConnected` and, for each of the first two clients, spawns a ship from its actor
  (`actors/ship_left.actor` or `ship_right.actor`: `game.Ship`, a `rtype.Transform`, a box `rtype.Collider` and a
  kinematic `rtype.RigidBody`) with `engine.capsule.spawn`, then adds `net.Owner` and `net.PlayerInput` with
  `ctx.commands:add`, as an override only changes what the actor has. A third client finds the game full. It removes the ship when it reads `net.ClientDisconnected`.
  See [Networking](networking.md).
- **`obstacles.luau`** copies the place of each obstacle the arena spawned into its replicated `game.Obstacle`, in
  `PostUpdate` before `net.snapshot`, for the clients to draw it there.
- **`ships.luau`** moves the ships in `FixedUpdate`, after `net.applyInputs` and before the physics syncs, reading the
  player's `move` action with `rtype.replication.readInput` and sliding along the obstacles with
  `rtype.physics.moveAndSlide`. On `shoot.pressed` it spawns `actors/laser.actor` (a dynamic body with a
  `rtype.Velocity`, no gravity and continuous collision), with overrides for its place, its shooter, its direction and
  the ship it can hit. One shot in ten misfires: a `game.Misfire` replaces the laser so the clients play
  another sound.
- **`lasers.luau`** copies the laser bodies' positions into the replicated `game.Laser` after the physics, removes
  those that left the view, and reads `rtype.CollisionStarted` events: a laser that touches something is spent, and
  one that touches a ship spawns a `game.Winner`. See [Physics](physics.md).

## The client

Every client file starts with `if not engine.isClient then return end` once it has declared what both sides need.

- **`assets.luau`** loads everything with the `engine.assets` forms and keeps the ids in `Duel.assets`. See
  [Setup forms](setup-forms.md).
- **`scene.luau`** spawns the camera (`rtype.Camera` and `rtype.AudioListener`) in a `Startup` system, gives each
  obstacle a `rtype.ShapeRenderer` (and the `rtype.Transform` the server's `game.Obstacle` says, when it came through
  the network) in `PostUpdate`, which also runs while the editor edits the arena, and spawns the space background map
  and the planets that pass.
- **`sprites.luau`** finds the new `game.Ship` and `game.Laser` entities with `added = {...}` and gives them a
  `rtype.SpriteRenderer` and `rtype.SpriteAnimator` from the atlases, and spawns the explosion for the loser.
- **`sounds.luau`** sends `rtype.PlayMusic` at start and calls `rtype.audio.playSound` for each new laser or misfire.
- **`hud.luau`** spawns `rtype.TextRenderer` entities for the names, and when `game.Winner` appears says who won, using
  `rtype.replication.localClient` and `net.Owner` to know whether this client is the winner (with the network only).
- **`controls.luau`** adds a local Escape action with engine calls instead of `engine.input.action`, so it is not sent
  to the server.

## What to take from it

1. The server decides everything that matters and writes replicated components; the clients only present them.
2. One folder per side, shared declarations in a file that sorts first.
3. Everything the engine does for the game is chosen in data: plugins and settings in `project.luau`, assets and
   inputs by forms, behavior by components and systems.
