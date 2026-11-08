-- What happened to the riders since the last frame, found once for whoever shows it (client/sounds.luau,
-- client/effects.luau): the shots fired, from a rider's game.Shot serial; the deaths, a rider hit when game.Shield
-- turns ghost, or a cyclist knocked off the road when its shooter's game.Score counts one more, where that shooter's
-- beam stopped; and how many riders the player's own shots hit, from its game.Score's kills. A rider seen for the
-- first time only sets what it was.

-- What each rider was at the last frame, by its entity's bits.
local seenOf: { [number]: { shots: number, ghost: boolean, kills: number, cyclists: number } } = {}

-- This frame's events, filled by game.DetectRiderEvents.
local shots: { { from: { x: number, y: number }, to: { x: number, y: number }, mine: boolean } } = {}
local hits: { { at: { x: number, y: number }, mine: boolean } } = {}
local kills = 0

-- Update: finds this frame's shots, hits and the player's kills.
engine.ecs.system("game.DetectRiderEvents", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Shot", "game.Shield", "game.Score", "game.Position", "net.Owner" } },
	calls = { "rtype.replication.localClient" },
	run = function(ctx)
		table.clear(shots)
		table.clear(hits)
		kills = 0
		local me = ctx.call("rtype.replication.localClient", {})
		for entity, shot, shield, score, position, owner in ctx.query do
			local seen = seenOf[entity.bits]
			seenOf[entity.bits] = {
				shots = shot.serial,
				ghost = shield.ghost,
				kills = score.kills,
				cyclists = score.cyclists,
			}
			if seen == nil then
				continue
			end
			local mine = me.connected and owner.client == me.client
			if shot.serial ~= seen.shots then
				table.insert(shots, { from = shot.from, to = shot.to, mine = mine })
			end
			if shield.ghost and not seen.ghost then
				table.insert(hits, { at = position.value, mine = mine })
			end
			if score.cyclists ~= seen.cyclists then
				table.insert(hits, { at = shot.to, mine = false }) -- a cyclist, where this rider's beam stopped
			end
			if mine and score.kills ~= seen.kills then
				kills += (score.kills - seen.kills) % 65536
			end
		end
	end,
})

return {
	--- The shots fired this frame: where each went from and to, and whether the player fired it.
	shots = function()
		return table.clone(shots)
	end,
	--- The deaths this frame, riders hit and cyclists knocked off: where each is, and whether it is the player's own.
	hits = function()
		return table.clone(hits)
	end,
	--- How many riders the player's own shots hit this frame.
	kills = function(): number
		return kills
	end,
}
