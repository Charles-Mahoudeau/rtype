-- The rules both sides share: the pitch, the feel of the game, the replicated components and the input actions. The
-- shared entry requires it, and the files of each side require it for the `Game` table; the manifest checks that both
-- sides declared the same components and actions.

local Game = {
	-- The view, in world units (project.luau sizes the window the same).
	viewWidth = 1280,
	viewHeight = 720,

	-- The pitch: its lines, the goals and the border players cannot cross. tools/make_art.py draws the same.
	pitchCenter = { x = 640, y = 396 },
	pitchSize = { x = 1100, y = 520 },
	goalHalfWidth = 85, -- from the pitch's middle line to each post
	goalDepth = 50,
	borderMargin = { x = 20, top = 76, bottom = 8 }, -- where players stop: the view's edges, under the scoreboard
	wallThickness = 40,

	-- The bodies.
	playerRadius = 20,
	ballRadius = 12,
	postRadius = 8,
	playerMass = 2,
	ballMass = 1,
	-- Damping caps the speed: a player's top speed is its acceleration divided by its damping.
	playerDamping = 2.4,
	ballDamping = 0.6,
	bounce = 0.5, -- the restitution of every body
	playerAcceleration = 520, -- world units per second squared
	kickingAcceleration = 380, -- slower while holding kick
	kickSpeed = 430, -- what a kick adds to the ball's speed, world units per second
	kickReach = 6, -- the gap between a player and the ball still in reach of a kick
	kickCooldown = 0.25, -- seconds between two kicks

	-- The match.
	teamSize = 4,
	matchDuration = 180, -- seconds
	scoreLimit = 5,
	countdown = 3, -- seconds before a kickoff
	goalPause = 2.5, -- seconds of celebration after a goal
	endPause = 6, -- seconds the result stays before a new match
	nameLength = 16,

	-- The teams, as game.Player.team says.
	spectators = 0,
	red = 1,
	blue = 2,
	teamNames = { [1] = "Red", [2] = "Blue" },

	-- The match's state, as game.Match.state says.
	waiting = 0, -- fewer than one player per team: free play, no score
	kickoff = 1, -- the countdown; nobody moves
	playing = 2,
	scored = 3, -- the celebration after a goal
	ended = 4,

	-- The collision categories: each Collider belongs to some (layer) and collides with some (mask).
	playerLayer = 1,
	ballLayer = 2,
	ballWallLayer = 4, -- the lines and the nets: only the ball stops there
	playerWallLayer = 8, -- the view's edges: only players stop there
	postLayer = 16,

	-- The enums of the engine's components, as their reflected numbers.
	boxShape = 0, -- rtype.Collider shape
	circleShape = 1,
	staticBody = 0, -- rtype.RigidBody type
	dynamicBody = 2,
}

-- Where the pitch's lines are.
Game.pitchLeft = Game.pitchCenter.x - Game.pitchSize.x / 2
Game.pitchRight = Game.pitchCenter.x + Game.pitchSize.x / 2
Game.pitchTop = Game.pitchCenter.y - Game.pitchSize.y / 2
Game.pitchBottom = Game.pitchCenter.y + Game.pitchSize.y / 2
Game.postTop = Game.pitchCenter.y - Game.goalHalfWidth
Game.postBottom = Game.pitchCenter.y + Game.goalHalfWidth

-- Where each team's players start, from the pitch's center, for the red team (on the left); blue is mirrored. The
-- team kicking off puts its first player closer to the ball.
local spots = { { x = -180, y = 0 }, { x = -320, y = -130 }, { x = -320, y = 130 }, { x = -470, y = 0 } }
local kickoffSpot = { x = -60, y = 0 }

--- Where the `index`-th player of a team (counted from 1) starts a kickoff.
function Game.spotOf(team: number, index: number, kicksOff: boolean): { x: number, y: number }
	local spot = if index == 1 and kicksOff then kickoffSpot else spots[(index - 1) % #spots + 1]
	local side = if team == Game.red then 1 else -1
	return { x = Game.pitchCenter.x + spot.x * side, y = Game.pitchCenter.y + spot.y }
end

--- The length of a vector.
function Game.length(vector: { x: number, y: number }): number
	return math.sqrt(vector.x * vector.x + vector.y * vector.y)
end

-- A player on the pitch, spawned when its client joins a team. Its position and velocity are copied from its body.
engine.ecs.component("game.Player", {
	fields = { team = "u8", name = "string", position = "vec2", velocity = "vec2", kicking = "bool" },
	replication = "everyChange",
})
-- The ball, copied from its body like the players.
engine.ecs.component("game.Ball", {
	fields = { position = "vec2", velocity = "vec2" },
	replication = "everyChange",
})
-- The match, on one entity: its state, the clock and the score. `winner` is the team that won once it ended.
engine.ecs.component("game.Match", {
	fields = {
		state = "u8",
		timeLeft = "f32",
		countdown = "f32",
		red = "u8",
		blue = "u8",
		overtime = "bool",
		winner = "u8",
	},
	defaults = { timeLeft = Game.matchDuration },
	replication = "everyChange",
})

-- A wall or a post of the pitch, sent once so that a client can place its collider (F3 shows it).
engine.ecs.component("game.Fixture", { fields = { position = "vec2" }, replication = "spawnOnly" })
-- Server only: a player's kick. A kick fires once per press, as soon as the ball is in reach while the key is held.
engine.ecs.component("game.Kicker", { fields = { armed = "bool", cooldown = "f32" }, presence = "serverOnly" })

-- The client's own pieces, declared on both sides like every component: what follows a player (its name, its kick
-- ring, the marker over this client's player), and the nodes of the HUD and the menu a system changes, by kind.
engine.ecs.component("game.NameTag", { fields = { player = "entity" }, presence = "clientOnly" })
engine.ecs.component("game.KickRing", { fields = { player = "entity" }, presence = "clientOnly" })
engine.ecs.component("game.Marker", { fields = { player = "entity" }, presence = "clientOnly" })
engine.ecs.component("game.HudNode", { fields = { kind = "u8" }, presence = "clientOnly" })

-- The actions both sides agree on: the clients send them, the server reads them. The keys are physical positions
-- named after the US layout: W A S D is Z Q S D on a French keyboard.
engine.input.action("move", "vector2", {
	bind = {
		{ up = "W", down = "S", left = "A", right = "D" },
		{ up = "Up", down = "Down", left = "Left", right = "Right" },
	},
})
engine.input.action("kick", "button", { bind = { "Space", "X" } })

return Game
