-- The rules both sides share: the roads, the replicated components and the input actions. The shared entry requires
-- it, and the files of each side require it for the `Game` table; the manifest checks that both sides declared the
-- same components and actions.

local Game = {
	tile = 64, -- pixels per tile: every size of the game is counted in tiles
	roadTiles = 3, -- a vehicle (2 tiles) drives in its middle, a cyclist (1 tile) on any of its tiles
	roadCount = 8,
	sidebarTiles = 1, -- the strip on the left, under the timer and the gauge
	viewHeight = 900, -- world units: with the 8 roads' width, 16:9, the shape of most screens
	roadSpeed = 600, -- world units per second the roads scroll down: the riders' speed along the course (roads.tmj)
	maxPlayers = 4, -- the next players watch
	minPlayers = 2, -- a match starts once this many ride
	matchDuration = 240, -- seconds: whoever has the best score then wins
	resultsDuration = 10, -- seconds the results stay before the next match waits for its players
	riderTiles = { x = 1, y = 2 },
	riderSpeed = 400, -- world units per second, in every direction
	startRoads = { 1, 3, 4, 6 }, -- the road each slot starts on, counted from 0 on the left
	fireCooldown = 0.25, -- seconds between two shots while an arrow is held
	beamLifetime = 0.2, -- seconds a shot's beam stays, long enough for a snapshot to carry it
	beamSquare = 32, -- world units: a beam is drawn as a row of squares this big

	-- The score, counted only while a match races.
	pointsPerSecond = 5, -- every rider, every second
	shotCost = 20, -- each shot fired
	hitReward = 40, -- each rider or cyclist a shot reaches
	hitLoss = 0.1, -- the share of its points a hit rider loses; the shooter takes it, the traffic takes it from nobody
	hitShield = 2, -- seconds a hit rider cannot be hit again

	-- The boosts a cyclist may carry (game.Cyclist.boost), given to the rider whose shot reaches it.
	noBoost = 0,
	pointsBoost = 1, -- Game.boostPoints at once
	speedBoost = 2, -- Game.boostSpeedFactor times faster for Game.boostSpeedSeconds
	shieldBoost = 3, -- invincible for Game.boostShieldSeconds
	sniperBoost = 4, -- shots go through everything for Game.boostSniperSeconds
	boostChance = 0.5, -- the share of cyclists that carry one
	boostPoints = 50,
	boostSpeedFactor = 1.5,
	boostSpeedSeconds = 15,
	boostShieldSeconds = 5,
	boostSniperSeconds = 5,

	-- The collision categories: each Collider belongs to some (layer) and collides with some (mask). A shot is a ray,
	-- which picks the categories it sees: it needs none of its own.
	riderLayer = 1,
	vehicleLayer = 2,
	cyclistLayer = 4,

	-- The draw layers: the beams far back, just over the roads, then the traffic (in its capsules), then the riders.
	beamDrawLayer = -50,
	trafficDrawLayer = 0,
	riderDrawLayer = 1,

	-- The phases of a match (game.Match).
	waiting = 0,
	racing = 1,
	over = 2,

	-- The capsules (assets/): what the server spawns for a player, and the scene around the roads. Their sizes and
	-- layers repeat the ones above: change both together.
	riderActor = "actors/rider.actor",
	roadsScene = "scenes/roads.scene",

	-- The enums of the engine's components, as their reflected numbers.
	box = 0, -- rtype.Collider shape
	rectShape = 0, -- rtype.ShapeRenderer kind
	circleShape = 1,
}

--- The size of something counted in tiles, in world units.
function Game.sizeOf(tiles: { x: number, y: number }): { x: number, y: number }
	return { x = tiles.x * Game.tile, y = tiles.y * Game.tile }
end

-- The sizes in world units, from the tiles. project.luau sizes the window the same, and tools/make_placeholders.py
-- draws the art the same.
Game.viewWidth = (Game.sidebarTiles + Game.roadCount * Game.roadTiles) * Game.tile
Game.roadsLeft = Game.sidebarTiles * Game.tile -- the roads' edges: the riders stay inside
Game.roadsRight = Game.viewWidth
Game.riderSize = Game.sizeOf(Game.riderTiles)
Game.startY = Game.viewHeight - Game.riderSize.y -- where the riders start: half a rider above the bottom

--- The X of a road's middle, the road counted from 0 on the left.
function Game.roadCenterX(road: number): number
	return Game.roadsLeft + (road + 0.5) * Game.roadTiles * Game.tile
end

--- Where the rider of a slot starts.
function Game.startOf(slot: number): { x: number, y: number }
	return { x = Game.roadCenterX(Game.startRoads[slot + 1]), y = Game.startY }
end

-- What moves and is drawn, riders and traffic alike: the server writes where it is, and the clients draw its look
-- there, at its collider's size (client/looks.luau).
engine.ecs.component("game.Position", { fields = { value = "vec2" }, replication = "everyChange" })
-- What it looks like, sent once: its texture, and its draw layer (Game.trafficDrawLayer...).
engine.ecs.component("game.Look", { fields = { texture = "assetId", layer = "i8" }, replication = "spawnOnly" })

-- A player's rider (assets/actors/rider.actor), sent once. Its slot (0 to 3) gives its start and its color.
engine.ecs.component("game.Rider", { fields = { slot = "u32" }, replication = "spawnOnly" })
-- A rider's points, on every change: written only when they change. Never below 0.
engine.ecs.component("game.Score", { fields = { points = "u32" }, replication = "everyChange" })
-- Whether a rider is invincible, on every change: written only when it turns on or off, and the clients make it
-- twinkle. Nothing hits a rider while it is on.
engine.ecs.component("game.Shield", { fields = { active = "bool" }, replication = "everyChange" })
-- Server only: the seconds before a rider's shield turns off.
engine.ecs.component("game.ShieldClock", { fields = { remaining = "f32" }, presence = "serverOnly" })
-- Server only: the seconds a rider's speed and sniper boosts still last.
engine.ecs.component("game.Boosts", { fields = { speed = "f32", sniper = "f32" }, presence = "serverOnly" })
-- Server only: the seconds before a rider can fire again.
engine.ecs.component("game.Gun", { fields = { cooldown = "f32" }, presence = "serverOnly" })
-- A shot's beam, an entity of its own sent once: it goes from the rider in slot `shooter` to what it hit, or to the
-- view's edge, and the clients draw it as a row of squares (client/beams.luau).
engine.ecs.component("game.Beam", {
	fields = { from = "vec2", to = "vec2", shooter = "u32" },
	replication = "spawnOnly",
})
-- Server only: the seconds before a beam is removed.
engine.ecs.component("game.BeamTimer", { fields = { remaining = "f32" }, presence = "serverOnly" })

-- The match, one entity, on every change: its phase (Game.waiting, racing or over) and the whole seconds left,
-- written only when they change, so that the clients get a new value once a second.
engine.ecs.component("game.Match", { fields = { phase = "u8", secondsLeft = "u16" }, replication = "everyChange" })
-- Server only: the exact seconds left in the match's phase.
engine.ecs.component("game.MatchClock", { fields = { remaining = "f32" }, presence = "serverOnly" })

-- Server only: a hit, an entity of its own: `victim` was hit this tick by the traffic (shooter -1) or by the shot of
-- the rider in slot `shooter`. Whatever hits a rider spawns one, and server/hits.luau resolves every hit in one place,
-- then despawns it.
engine.ecs.component("game.Hit", {
	fields = { victim = "entity", shooter = "i32" },
	defaults = { shooter = -1 },
	presence = "serverOnly",
})

-- The traffic: one capsule file per kind in assets/actors/, which gives its size (its collider), its look and its
-- speed. Server only: how fast it goes up the view, in world units per second. Slow: the traffic overtakes the riders.
engine.ecs.component("game.Driving", { fields = { speed = "f32" }, presence = "serverOnly" })
-- The vehicles that are cyclists, sent once: the boost a shot that hits it gives (Game.noBoost...), which the clients
-- show by its color, and the gauge follows them.
engine.ecs.component("game.Cyclist", { fields = { boost = "u8" }, replication = "spawnOnly" })

-- A text of the HUD (client/hud.luau), client only: what it shows (its role) and, for a score, whose (its slot).
engine.ecs.component("game.HudText", { fields = { role = "u8", slot = "u8" }, presence = "clientOnly" })

-- A square of a shot's beam (client/beams.luau), client only: the seconds it is still shown.
engine.ecs.component("game.BeamSquare", { fields = { remaining = "f32" }, presence = "clientOnly" })

-- The race's gauge (client/gauge.luau), client only: its orange part, and the dot of a cyclist.
engine.ecs.component("game.GaugeFill", { fields = {}, presence = "clientOnly" })
engine.ecs.component("game.GaugeDot", { fields = { cyclist = "entity" }, presence = "clientOnly" })

-- Markers of the scene's client nodes (assets/scenes/roads.scene): a server reads the file too, and the camera and the
-- shape are not its components, so the client gives them to the marked entities.
engine.ecs.component("game.Camera", { fields = {}, presence = "clientOnly" })
engine.ecs.component("game.Sidebar", { fields = {}, presence = "clientOnly" })

-- The actions both sides agree on: the clients send them, the server reads them. The keys are physical positions
-- named after the US layout: W A S D is Z Q S D on a French keyboard.
engine.input.action("move", "vector2", { bind = { { up = "W", down = "S", left = "A", right = "D" } } })
engine.input.action("fire", "vector2", { bind = { { up = "Up", down = "Down", left = "Left", right = "Right" } } })

return Game
