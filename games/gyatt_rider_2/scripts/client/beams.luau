-- The shots' beams: a row of white squares from the shooter to what the shot hit, far back under the traffic and the
-- riders. The squares are children of the beam, so they leave with it, and they also leave on their own after
-- Game.beamLifetime: the server can remove a beam before the client confirmed it, and the client then never hears of
-- it (engine issue).

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")

-- Update: draws each new beam the server sent as squares side by side along it.
engine.ecs.system("game.DrawBeams", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Beam" }, without = { "rtype.Transform" } },
	run = function(ctx)
		local size = Game.beamSquare
		for entity, beam in ctx.query do
			local along = { x = beam.to.x - beam.from.x, y = beam.to.y - beam.from.y }
			local length = math.sqrt(along.x * along.x + along.y * along.y)
			local direction = if length > 0 then { x = along.x / length, y = along.y / length } else { x = 0, y = 0 }
			ctx.commands:add(entity, "rtype.Transform", { position = beam.from })
			for index = 0, math.max(1, math.round(length / size)) - 1 do
				local offset = (index + 0.5) * size -- along the beam, from its start: the square's middle
				local square = ctx.commands:spawn({
					["game.BeamSquare"] = { remaining = Game.beamLifetime },
					["rtype.Transform"] = { position = { x = direction.x * offset, y = direction.y * offset } },
					["rtype.SpriteRenderer"] = {
						texture = assets.beam,
						source = { x = 0, y = 0, z = size, w = size }, -- the whole image
						origin = { x = size / 2, y = size / 2 },
						layer = Game.beamDrawLayer,
					},
				})
				ctx.commands:setParent(square, entity)
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
