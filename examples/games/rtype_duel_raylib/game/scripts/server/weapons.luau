-- What the ships shoot: a laser, or one time in Duel.misfireOdds a misfire.

local Duel = require("@scripts/shared/arena")

local Weapons = {}

--- Fires a laser from a player's ship at @p position, or misfires.
function Weapons.fire(ctx, player: number, position)
	local direction = Duel.directionOf(player)
	local muzzle = {
		x = position.x + direction * (Duel.shipHalfSize.x + Duel.laserHalfSize.x + 1),
		y = position.y,
	}
	if math.random(1, Duel.misfireOdds) == 1 then
		ctx.commands:spawn({
			["game.Misfire"] = { position = muzzle, shooter = player },
			["game.MisfireTimer"] = { remaining = Duel.misfireLifetime },
		})
		return
	end
	-- The laser's actor (assets/actors/laser.actor) is the body; the shot gives where, who, which way and whom it hits.
	engine.capsule.spawn(ctx, Duel.laser, {
		["game.Laser"] = { position = muzzle, shooter = player },
		["rtype.Transform"] = { position = muzzle },
		["rtype.Collider"] = { mask = bit32.bor(Duel.obstacleLayer, Duel.shipLayerOf(1 - player)) },
		["rtype.Velocity"] = { linear = { x = direction * Duel.laserSpeed, y = 0 } },
	})
end

return table.freeze(Weapons)
