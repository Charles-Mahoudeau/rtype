-- The boosts a rider wins by shooting a cyclist that carries one: points at once, a shield through Shield.grant, or
-- seconds of speed or of sniper shots, which game.WearBoosts counts down.

local Game = require("@scripts/shared/rules")
local Shield = require("@scripts/server/shield")

-- FixedUpdate, before the riders move and shoot: the speed and sniper boosts wear off.
engine.ecs.system("game.WearBoosts", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.Boosts" } },
	before = { "game.MoveRiders" },
	run = function(ctx)
		for _, boosts in ctx.query do
			if boosts.speed > 0 then
				boosts.speed = math.max(0, boosts.speed - ctx.time.fixedDelta)
			end
			if boosts.sniper > 0 then
				boosts.sniper = math.max(0, boosts.sniper - ctx.time.fixedDelta)
			end
		end
	end,
})

return {
	--- Gives @p boost to a rider, from a system that writes its score, its shield and its boosts. @p counts says
	--- whether points count (the match races).
	give = function(boost: number, counts: boolean, score, shield, clock, boosts)
		if boost == Game.pointsBoost then
			if counts then
				score.points += Game.boostPoints
			end
		elseif boost == Game.speedBoost then
			boosts.speed = Game.boostSpeedSeconds
		elseif boost == Game.shieldBoost then
			Shield.grant(shield, clock, Game.boostShieldSeconds)
		elseif boost == Game.sniperBoost then
			boosts.sniper = Game.boostSniperSeconds
		end
	end,
	--- How many times faster the boosts make a rider go.
	speedFactor = function(boosts): number
		return if boosts.speed > 0 then Game.boostSpeedFactor else 1
	end,
	--- Whether a rider's shots go through everything.
	sniping = function(boosts): boolean
		return boosts.sniper > 0
	end,
}
