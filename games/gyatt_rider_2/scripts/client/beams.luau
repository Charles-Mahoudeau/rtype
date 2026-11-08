-- The shots' beams: when a rider's game.Shot counts a new shot, a row of white squares from where it fired to what the
-- shot hit, far back under the traffic and the riders, each square leaving on its own after Game.beamLifetime.

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")

-- The last shot drawn of each rider, by its entity's bits: its game.Shot's serial.
local drawnOf: { [number]: number } = {}

-- Update: draws each rider's new shot as squares side by side along its beam.
engine.ecs.system("game.DrawBeams", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Shot" } },
	run = function(ctx)
		local size = Game.beamSquare
		for entity, shot in ctx.query do
			local drawn = drawnOf[entity.bits]
			drawnOf[entity.bits] = shot.serial
			if drawn == nil or drawn == shot.serial then
				continue -- a rider just seen, or no new shot
			end
			local along = { x = shot.to.x - shot.from.x, y = shot.to.y - shot.from.y }
			local length = math.sqrt(along.x * along.x + along.y * along.y)
			local direction = if length > 0 then { x = along.x / length, y = along.y / length } else { x = 0, y = 0 }
			for index = 0, math.max(1, math.round(length / size)) - 1 do
				local offset = (index + 0.5) * size -- along the beam, from its start: the square's middle
				ctx.commands:spawn({
					["game.BeamSquare"] = { remaining = Game.beamLifetime },
					["rtype.Transform"] = {
						position = { x = shot.from.x + direction.x * offset, y = shot.from.y + direction.y * offset },
					},
					["rtype.SpriteRenderer"] = {
						texture = assets.beam,
						source = { x = 0, y = 0, z = size, w = size }, -- the whole image
						origin = { x = size / 2, y = size / 2 },
						layer = Game.beamDrawLayer,
					},
				})
			end
		end
	end,
})

-- Update: a square leaves once it was shown for Game.beamLifetime.
engine.ecs.system("game.FadeBeams", {
	phase = "Update",
	side = "client",
	query = { write = { "game.BeamSquare" } },
	run = function(ctx)
		for entity, square in ctx.query do
			square.remaining -= ctx.time.delta
			if square.remaining <= 0 then
				ctx.commands:despawn(entity)
			end
		end
	end,
})
