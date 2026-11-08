-- The pitch's bodies: the lines and the nets stop only the ball, the view's edges stop only the players, the posts stop
-- both. Then the ball, at the center, and the match.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/server/state")

type Vec2 = { x: number, y: number }

local thick = Game.wallThickness
local ballOnly = { layer = Game.ballWallLayer, mask = Game.ballLayer }
local playersOnly = { layer = Game.playerWallLayer, mask = Game.playerLayer }

--- A static box from its top-left to its bottom-right corner, in the given collision categories.
local function box(ctx: any, from: Vec2, to: Vec2, filter: { layer: number, mask: number })
	local position = { x = (from.x + to.x) / 2, y = (from.y + to.y) / 2 }
	ctx.commands:spawn({
		["game.Fixture"] = { position = position },
		["rtype.Transform"] = { position = position },
		["rtype.Collider"] = {
			shape = Game.boxShape,
			size = { x = to.x - from.x, y = to.y - from.y },
			restitution = Game.bounce,
			friction = 0,
			layer = filter.layer,
			mask = filter.mask,
		},
	})
end

local function post(ctx: any, position: Vec2)
	ctx.commands:spawn({
		["game.Fixture"] = { position = position },
		["rtype.Transform"] = { position = position },
		["rtype.Collider"] = {
			shape = Game.circleShape,
			radius = Game.postRadius,
			restitution = Game.bounce,
			friction = 0,
			layer = Game.postLayer,
			mask = bit32.bor(Game.playerLayer, Game.ballLayer),
		},
	})
end

-- The lines and the net of one side: `x` is its goal line, `outward` -1 on the left and 1 on the right.
local function side(ctx: any, x: number, outward: number)
	local function span(a: number, b: number): (number, number)
		return math.min(a, b), math.max(a, b)
	end
	local lineFrom, lineTo = span(x, x + outward * thick)
	-- The line above and below the goal.
	box(ctx, { x = lineFrom, y = Game.pitchTop - thick }, { x = lineTo, y = Game.postTop }, ballOnly)
	box(ctx, { x = lineFrom, y = Game.postBottom }, { x = lineTo, y = Game.pitchBottom + thick }, ballOnly)
	-- The net: its two sides and its back.
	local back = x + outward * Game.goalDepth
	local netFrom, netTo = span(x, back)
	box(ctx, { x = netFrom, y = Game.postTop - thick }, { x = netTo, y = Game.postTop }, ballOnly)
	box(ctx, { x = netFrom, y = Game.postBottom }, { x = netTo, y = Game.postBottom + thick }, ballOnly)
	local backFrom, backTo = span(back, back + outward * thick)
	box(ctx, { x = backFrom, y = Game.postTop - thick }, { x = backTo, y = Game.postBottom + thick }, ballOnly)
	-- The posts.
	post(ctx, { x = x, y = Game.postTop })
	post(ctx, { x = x, y = Game.postBottom })
end

-- Startup: the pitch, the ball and the match.
engine.ecs.system("game.SpawnPitch", {
	phase = "Startup",
	side = "server",
	query = {},
	run = function(ctx)
		-- The touchlines, along the top and the bottom.
		box(ctx, { x = Game.pitchLeft - thick, y = Game.pitchTop - thick }, { x = Game.pitchRight + thick, y = Game.pitchTop }, ballOnly)
		box(ctx, { x = Game.pitchLeft - thick, y = Game.pitchBottom }, { x = Game.pitchRight + thick, y = Game.pitchBottom + thick }, ballOnly)
		side(ctx, Game.pitchLeft, -1)
		side(ctx, Game.pitchRight, 1)

		-- The view's edges, for the players.
		local left, right = Game.borderMargin.x, Game.viewWidth - Game.borderMargin.x
		local top, bottom = Game.borderMargin.top, Game.viewHeight - Game.borderMargin.bottom
		box(ctx, { x = left - thick, y = top - thick }, { x = right + thick, y = top }, playersOnly)
		box(ctx, { x = left - thick, y = bottom }, { x = right + thick, y = bottom + thick }, playersOnly)
		box(ctx, { x = left - thick, y = top }, { x = left, y = bottom }, playersOnly)
		box(ctx, { x = right, y = top }, { x = right + thick, y = bottom }, playersOnly)

		State.setBall(ctx.commands:spawn({
			["game.Ball"] = { position = Game.pitchCenter },
			["rtype.Transform"] = { position = Game.pitchCenter },
			["rtype.Collider"] = {
				shape = Game.circleShape,
				radius = Game.ballRadius,
				restitution = Game.bounce,
				friction = 0,
				layer = Game.ballLayer,
				mask = bit32.bor(Game.playerLayer, Game.ballWallLayer, Game.postLayer),
			},
			["rtype.RigidBody"] = {
				type = Game.dynamicBody,
				mass = Game.ballMass,
				linearDamping = Game.ballDamping,
				gravityScale = 0,
				fixedRotation = true,
				continuousCollision = true,
			},
			["rtype.Velocity"] = {},
		}))

		ctx.commands:spawn({ ["game.Match"] = {} })
	end,
})
