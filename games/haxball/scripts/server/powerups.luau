-- The power-ups: while a match is played, one appears now and then on the pitch at a random place, and the first player
-- to touch it gets it for Game.powerupDuration seconds (speed, big, no inertia, strong kick). Taking a power-up again
-- starts its clock over. A kickoff clears the pitch and the players.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/server/state")

local nextSpawn = math.random() * (Game.powerupInterval.max - Game.powerupInterval.min) + Game.powerupInterval.min
local clearedFor = 0

--- Seconds to wait for the next power-up, at random.
local function interval(): number
	return Game.powerupInterval.min + math.random() * (Game.powerupInterval.max - Game.powerupInterval.min)
end

-- FixedUpdate: the power-ups appearing while the ball is in play, and disappearing at a kickoff.
engine.ecs.system("game.SpawnPowerups", {
	phase = "FixedUpdate",
	side = "server",
	query = {},
	run = function(ctx)
		local match = State.match()
		local powerups = State.powerups()
		if match.resets ~= clearedFor then
			clearedFor = match.resets
			for _, powerup in powerups do
				ctx.commands:despawn(powerup.entity)
			end
			table.clear(powerups)
			nextSpawn = interval()
		end
		if match.state ~= Game.playing then
			return
		end
		nextSpawn -= ctx.time.fixedDelta
		if nextSpawn > 0 then
			return
		end
		nextSpawn = interval()
		if #powerups >= Game.maxPowerups then
			return
		end
		local margin = 90 -- away from the goals and the lines
		local position = {
			x = Game.pitchLeft + margin + math.random() * (Game.pitchSize.x - 2 * margin),
			y = Game.pitchTop + 40 + math.random() * (Game.pitchSize.y - 80),
		}
		local kind = math.random(1, 4)
		local entity = ctx.commands:spawn({ ["game.Powerup"] = { kind = kind, position = position } })
		table.insert(powerups, { entity = entity, kind = kind, position = position })
	end,
})

-- FixedUpdate, once the players moved: a player touching a power-up takes it. At a kickoff the players lose theirs.
local buffsClearedFor = 0
engine.ecs.system("game.CollectPowerups", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "rtype.Transform", "game.Player" }, write = { "game.Buffs" } },
	calls = { "rtype.net.sendToClients" },
	after = { "game.MovePlayers", "game.MirrorPlayers", "game.BallContacts", "game.RunMatch" },
	run = function(ctx)
		local match = State.match()
		local powerups = State.powerups()
		local reset = match.resets ~= buffsClearedFor
		buffsClearedFor = match.resets
		for _, transform, player, buffs in ctx.query do
			if reset then
				buffs.speed, buffs.big, buffs.direct, buffs.strongKick = 0, 0, 0, 0
				continue
			end
			local radius = Game.playerRadius * (if Game.hasBuff(player.buffs, Game.big) then Game.bigFactor else 1)
			local reach = radius + Game.powerupRadius
			for index = #powerups, 1, -1 do
				local powerup = powerups[index]
				local dx = powerup.position.x - transform.position.x
				local dy = powerup.position.y - transform.position.y
				if dx * dx + dy * dy > reach * reach then
					continue
				end
				if powerup.kind == Game.speed then
					buffs.speed = Game.powerupDuration
				elseif powerup.kind == Game.big then
					buffs.big = Game.powerupDuration
				elseif powerup.kind == Game.direct then
					buffs.direct = Game.powerupDuration
				else
					buffs.strongKick = Game.powerupDuration
				end
				ctx.commands:despawn(powerup.entity)
				table.remove(powerups, index)
				engine.net.sendToClients(ctx, "game.PowerupTaken", {
					kind = powerup.kind,
					x = powerup.position.x,
					y = powerup.position.y,
				})
				print(`[server] {player.name} takes {Game.powerupNames[powerup.kind]}`)
			end
		end
	end,
})
