-- The ball: copied to game.Ball for the clients, put back on the center spot for a kickoff, and its contacts: the last
-- player to touch it scores, and a bounce off a post or a line is sent to the clients for its sparks and its sound.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/server/state")

local bounceSpeed = 80 -- slower contacts make no spark

-- FixedUpdate, once the physics moved it: copies the ball to game.Ball.
engine.ecs.system("game.MirrorBall", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "rtype.Transform", "rtype.Velocity" }, write = { "game.Ball" } },
	after = { "rtype.Physics.SyncFromPhysics" },
	run = function(ctx)
		for _, transform, velocity, ball in ctx.query do
			ball.position = transform.position
			ball.velocity = velocity.linear
			State.ballMoved(transform.position, velocity.linear)
		end
	end,
})

-- FixedUpdate, before the bodies move: the ball goes back to the center spot when the match asks for it, and waits
-- there during the countdown.
local ballReset = 0
engine.ecs.system("game.ResetBall", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "rtype.Transform", "rtype.Velocity" }, with = { "game.Ball" } },
	after = { "rtype.Physics.SyncBodies", "game.MovePlayers", "game.ResetPlayers" },
	before = { "rtype.Physics.SyncToPhysics" },
	run = function(ctx)
		local match = State.match()
		if match.state ~= Game.kickoff and match.resets == ballReset then
			return
		end
		ballReset = match.resets
		for _, transform, velocity in ctx.query do
			transform.position = Game.pitchCenter
			velocity.linear = { x = 0, y = 0 }
		end
	end,
})

-- FixedUpdate, once the physics told what touched: who touched the ball last, and its bounces.
engine.ecs.system("game.BallContacts", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Player" } },
	events = { read = { "rtype.CollisionStarted" } },
	calls = { "rtype.net.sendToClients" },
	after = { "rtype.Physics.Events", "game.MirrorBall", "game.MirrorPlayers" },
	run = function(ctx)
		local ball = State.ball()
		if ball == nil then
			return
		end
		local touches = {}
		for collision in ctx.events.read("rtype.CollisionStarted") do
			if collision.first.bits == ball.bits then
				table.insert(touches, collision.second.bits)
			elseif collision.second.bits == ball.bits then
				table.insert(touches, collision.first.bits)
			end
		end
		if #touches == 0 then
			return
		end

		local players = {}
		for entity, player in ctx.query do
			players[entity.bits] = { name = player.name, team = player.team }
		end
		local position = State.ballPosition()
		local speed = Game.length(State.ballVelocity())
		for _, other in touches do
			local player = players[other]
			if player ~= nil then
				State.touched(player.name, player.team)
			elseif speed >= bounceSpeed then
				engine.net.sendToClients(ctx, "game.Bounce", { x = position.x, y = position.y, speed = speed })
			end
		end
	end,
})
