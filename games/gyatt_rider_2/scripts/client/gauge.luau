-- The race's gauge, in the strip on the left: the course from its start (bottom) to its finish (top, the end of the
-- match), its covered part in orange as the time passes, and a small orange cyclist riding for each cyclist at its
-- place on the course: the riders' place, plus how far ahead of them it is on the road.

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")
local Blocks = require("@scripts/client/blocks")

local gaugeX = Game.roadsLeft / 2
local gaugeTop, gaugeBottom = 64, Game.viewHeight - 16 -- under the timer, above the bottom
local gaugeWidth = 24
local gaugeHeight = gaugeBottom - gaugeTop
local courseLength = Game.roadSpeed * Game.matchDuration -- world units the riders cover in a match
local trackColor = { x = 0.2, y = 0.2, z = 0.22, w = 1 }
local fillColor = { x = 1, y = 0.55, z = 0, w = 1 }
local finishColor = { x = 1, y = 1, z = 1, w = 1 }
local dotColor = { x = 1, y = 0.35, z = 0, w = 1 } -- orange, deeper than the covered part
local dotScale = 0.45 -- of the cyclist's 64x128 image
-- The draw layers, over the strip's (10): the track, then its orange part and the finish line, then the cyclists. All
-- are sprites, the track and its parts blocks (client/blocks.luau): a shape would draw over the cyclists.
local trackLayer, fillLayer, dotLayer = 11, 12, 13

-- The covered share of the course (0 to 1), each cyclist's place on it this frame by its bits, and the cyclists that
-- have a dot: filled by game.ReadCourse and game.PlaceCyclists for the systems that draw.
local covered = 0
local placeOf: { [number]: number } = {}
local dotted: { [number]: boolean } = {}

--- The height on the gauge of @p share of the course.
local function heightOf(share: number): number
	return gaugeHeight * math.clamp(share, 0, 1)
end

-- Startup: the gauge's track, its finish line and its orange part, empty.
engine.ecs.system("game.SpawnGauge", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		ctx.commands:spawn({
			["rtype.Transform"] = {
				position = { x = gaugeX, y = (gaugeTop + gaugeBottom) / 2 },
				scale = Blocks.scale({ x = gaugeWidth, y = gaugeHeight }),
			},
			["rtype.SpriteRenderer"] = Blocks.sprite(trackColor, trackLayer),
		})
		ctx.commands:spawn({
			["rtype.Transform"] = {
				position = { x = gaugeX, y = gaugeTop },
				scale = Blocks.scale({ x = gaugeWidth + 16, y = 4 }),
			},
			["rtype.SpriteRenderer"] = Blocks.sprite(finishColor, fillLayer),
		})
		ctx.commands:spawn({
			["game.GaugeFill"] = {},
			["rtype.Transform"] = {
				position = { x = gaugeX, y = gaugeBottom },
				scale = Blocks.scale({ x = gaugeWidth, y = 0 }),
			},
			["rtype.SpriteRenderer"] = Blocks.sprite(fillColor, fillLayer),
		})
	end,
})

-- Update: how much of the course the match covered: none while it waits, all once it is over.
engine.ecs.system("game.ReadCourse", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Match" } },
	run = function(ctx)
		for _, match in ctx.query do
			if match.phase == Game.racing then
				covered = (Game.matchDuration - match.secondsLeft) / Game.matchDuration
			else
				covered = if match.phase == Game.over then 1 else 0
			end
		end
	end,
})

-- Update: each cyclist's place on the course, and a small cyclist on the gauge for each new one.
engine.ecs.system("game.PlaceCyclists", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Position" }, with = { "game.Cyclist" } },
	after = { "game.ReadCourse" },
	run = function(ctx)
		table.clear(placeOf)
		for entity, position in ctx.query do
			placeOf[entity.bits] = covered + (Game.startY - position.value.y) / courseLength
			if not dotted[entity.bits] then
				dotted[entity.bits] = true
				ctx.commands:spawn({
					["game.GaugeDot"] = { cyclist = entity },
					["rtype.Transform"] = {
						position = { x = gaugeX, y = gaugeBottom },
						scale = { x = dotScale, y = dotScale },
					},
					["rtype.SpriteRenderer"] = {
						texture = assets.cyclistSheet,
						source = { x = 0, y = 0, z = 64, w = 128 },
						origin = { x = 32, y = 64 },
						color = dotColor,
						layer = dotLayer,
					},
					["rtype.SpriteAnimator"] = { atlas = assets.cyclist, animation = "ride" },
				})
			end
		end
	end,
})

-- Update: the orange part grows to the covered share of the course.
engine.ecs.system("game.FillGauge", {
	phase = "Update",
	side = "client",
	query = { write = { "rtype.Transform" }, with = { "game.GaugeFill" } },
	after = { "game.ReadCourse", "game.FollowPositions" },
	run = function(ctx)
		local height = heightOf(covered)
		local scale = Blocks.scale({ x = gaugeWidth, y = height })
		for _, transform in ctx.query do
			if transform.scale.y ~= scale.y then
				transform.scale = scale
				transform.position = { x = gaugeX, y = gaugeBottom - height / 2 }
			end
		end
	end,
})

-- Update: each small cyclist goes to its cyclist's place, and leaves with it.
engine.ecs.system("game.MoveGaugeDots", {
	phase = "Update",
	side = "client",
	query = { read = { "game.GaugeDot" }, write = { "rtype.Transform" } },
	after = { "game.PlaceCyclists", "game.FillGauge" },
	run = function(ctx)
		for entity, dot, transform in ctx.query do
			local place = placeOf[dot.cyclist.bits]
			if place == nil then
				dotted[dot.cyclist.bits] = nil
				ctx.commands:despawn(entity)
				continue
			end
			transform.position = { x = gaugeX, y = gaugeBottom - heightOf(place) }
		end
	end,
})
