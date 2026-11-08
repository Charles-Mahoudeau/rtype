-- The shield: a rider that is hit, or that takes the invincibility boost, cannot be hit for a while. Whatever gives it
-- calls Shield.grant from a system that writes the rider's game.Shield and game.ShieldClock; game.LowerShields turns it
-- off once its time is out.

local Game = require("@scripts/shared/rules")

-- FixedUpdate, before this tick's hits are resolved: a shield whose time is out turns off. The shield is read, and
-- patched only when it turns off: a system that goes through a component in `write` marks it changed, which sends it
-- every tick.
engine.ecs.system("game.LowerShields", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Shield" }, write = { "game.ShieldClock" }, with = { "game.Rider" } },
	before = { "game.ResolveRiderHits" },
	run = function(ctx)
		for entity, shield, clock in ctx.query do
			if not shield.active then
				continue
			end
			clock.remaining -= ctx.time.fixedDelta
			if clock.remaining <= 0 then
				ctx.commands:patch(entity, "game.Shield", { active = false })
			end
		end
	end,
})

return {
	--- Shields a rider for @p seconds, or longer if it already was for longer.
	grant = function(shield, clock, seconds: number)
		if shield.active then
			clock.remaining = math.max(clock.remaining, seconds)
			return
		end
		shield.active = true
		clock.remaining = seconds
	end,
	--- How long a hit shields a rider.
	afterHit = Game.hitShield,
}
