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
	trafficSyncTicks = 18, -- ticks (0.3 s) between two positions a vehicle sends; the clients move it meanwhile
	maxPlayers = 4, -- the next players watch
	minPlayers = 2, -- a match starts once this many ride
	matchDuration = 120, -- seconds: whoever has the best score then wins
	resultsDuration = 10, -- seconds the results stay before the next match waits for its players
	riderTiles = { x = 1, y = 2 },
	riderSpeed = 400, -- world units per second, in every direction
	startRoads = { 1, 3, 4, 6 }, -- the road each slot starts on, counted from 0 on the left
	fireCooldown = 0.25, -- seconds between two shots while an arrow is held
	beamLifetime = 0.13, -- seconds a shot's beam stays on the clients' screens
	-- The replicated entities the game keeps at once at most: each snapshot sends every entity whole and holds 9 of
	-- them, and the ones left out never reach the clients (engine issue), so the traffic stays under it.
	maxReplicated = 9,
	beamSquare = 52, -- world units: a beam is drawn as a row of squares this big, and hits what it covers
	hitboxMargin = 16, -- world units: each hitbox (the capsules' colliders) is this much smaller than its image on a side

	-- The score, counted only while a match races.
	pointsPerSecond = 5, -- every rider, every second
	shotCost = 20, -- each shot fired; a rider with fewer points cannot fire
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
-- there (client/looks.luau).
engine.ecs.component("game.Position", { fields = { value = "vec2" }, replication = "everyChange" })
-- What it looks like, sent once: its texture, the size of its image in world units (a bit bigger than its hitbox),
-- and its draw layer (Game.trafficDrawLayer...).
engine.ecs.component("game.Look", {
	fields = { texture = "assetId", size = "vec2", layer = "i8" },
	replication = "spawnOnly",
})

-- A player's rider (assets/actors/rider.actor), sent once. Its slot (0 to 3) gives its start and its color.
engine.ecs.component("game.Rider", { fields = { slot = "u32" }, replication = "spawnOnly" })
-- A rider's points, on every change: written only when they change. Never below 0. And how many riders and cyclists
-- its shots hit, which tells the clients a kill and a cyclist knocked off the road (client/riderevents.luau).
engine.ecs.component("game.Score", {
	fields = { points = "u32", kills = "u16", cyclists = "u16" },
	replication = "everyChange",
})
-- Whether a rider is invincible, on every change: written only when it turns on or off, and the clients make it
-- twinkle. Nothing hits a rider while it is on.
-- A ghost (shielded by a hit) cannot shoot either; an invincible rider (the boost) can.
engine.ecs.component("game.Shield", { fields = { active = "bool", ghost = "bool" }, replication = "everyChange" })
-- Server only: the seconds before a rider's shield turns off.
engine.ecs.component("game.ShieldClock", { fields = { remaining = "f32" }, presence = "serverOnly" })
-- Server only: the seconds a rider's speed and sniper boosts still last, and how many points boosts it won.
engine.ecs.component("game.Boosts", {
	fields = { speed = "f32", sniper = "f32", points = "u16" },
	presence = "serverOnly",
})
-- The boosts a rider has on, on every change: written only when a boost starts or ends (server/boosts.luau), and the
-- clients show each with its effect (client/boostfx.luau). `points` counts the points boosts it won: a new one
-- twinkles. The shield boost is game.Shield, on and not a ghost.
engine.ecs.component("game.Boosted", {
	fields = { speed = "bool", sniper = "bool", points = "u16" },
	replication = "everyChange",
})
-- Server only: the seconds before a rider can fire again.
engine.ecs.component("game.Gun", { fields = { cooldown = "f32" }, presence = "serverOnly" })
-- A rider's last shot, on every change: from where to where its beam went, and how many shots it fired, which tells
-- the clients a new one to draw (client/beams.luau). A component of the rider rather than an entity per shot, which
-- the snapshots had no room for.
engine.ecs.component("game.Shot", {
	fields = { from = "vec2", to = "vec2", serial = "u16" },
	replication = "everyChange",
})

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
-- How fast a vehicle goes up the view right now, on every change: slower than its game.Driving behind slower traffic.
-- The clients move it at that speed between the positions it sends, every Game.trafficSyncTicks ticks: sending its
-- position every tick filled the snapshots, and the clients then lost spawns (invisible cars, missing beams).
engine.ecs.component("game.Speed", { fields = { value = "f32" }, replication = "everyChange" })
-- The vehicles that are cyclists, sent once: the boost a shot that hits it gives (Game.noBoost...), which the clients
-- show by its color, and the gauge follows them.
engine.ecs.component("game.Cyclist", { fields = { boost = "u8" }, replication = "spawnOnly" })

-- A text of the HUD (client/hud.luau), client only: what it shows (its role) and, for a score, whose (its slot).
engine.ecs.component("game.HudText", { fields = { role = "u8", slot = "u8" }, presence = "clientOnly" })

-- A square of a shot's beam (client/beams.luau), client only: the seconds it is still shown.
engine.ecs.component("game.BeamSquare", { fields = { remaining = "f32" }, presence = "clientOnly" })

-- The race's gauge (client/gauge.luau), client only: its orange part, and the small cyclist standing for a cyclist.
engine.ecs.component("game.GaugeFill", { fields = {}, presence = "clientOnly" })
engine.ecs.component("game.GaugeDot", { fields = { cyclist = "entity" }, presence = "clientOnly" })

-- The emitter of a boost's effect on a rider (client/boostfx.luau), client only: the rider, the boost (Game.speedBoost,
-- shieldBoost or sniperBoost), and the seconds its last particles still live once it stopped.
engine.ecs.component("game.BoostFx", {
	fields = { rider = "entity", boost = "u8", left = "f32" },
	presence = "clientOnly",
})

-- Markers of the scene's client nodes (assets/scenes/roads.scene): a server reads the file too, and the camera and the
-- shape are not its components, so the client gives them to the marked entities.
engine.ecs.component("game.Camera", { fields = {}, presence = "clientOnly" })
engine.ecs.component("game.Sidebar", { fields = {}, presence = "clientOnly" })

-- The actions both sides agree on: the clients send them, the server reads them. The keys are physical positions
-- named after the US layout: W A S D is Z Q S D on a French keyboard.
engine.input.action("move", "vector2", { bind = { { up = "W", down = "S", left = "A", right = "D" } } })
engine.input.action("fire", "vector2", { bind = { { up = "Up", down = "Down", left = "Left", right = "Right" } } })

return Game
