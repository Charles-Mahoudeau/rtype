-- The look of what the server replicates: the players (their disc, their name, the ring while they hold kick, the
-- marker over this client's own player) and the ball, spinning as it rolls. Between two snapshots a body goes on along
-- its velocity, so it moves smoothly at any frame rate.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/client/state")
local assets = require("@scripts/client/assets")

local playerDrawLayer = 2
local ballDrawLayer = 3
local ringDrawLayer = 4
local tagDrawLayer = 5
local playerSize = (Game.playerRadius + 4) * 2 -- the images have 4 pixels around the disc
local ballSize = (Game.ballRadius + 4) * 2
local nameOffset = Game.playerRadius + 6
local markerOffset = Game.playerRadius + 18
local iconDrawLayer = 5
local powerupDrawLayer = 1
local iconSize = 16 -- the icons over a player, from the 32 pixels of the sheet
local iconStep = 18
local iconOffset = Game.playerRadius + 38
local maxLead = 0.05 -- seconds a body goes on along its velocity, at most, without a new snapshot

type Vec2 = { x: number, y: number }

-- What the players' pieces need, by the player's entity bits, filled by game.ShowPlayers.
local names: { [number]: string } = {}
local kicking: { [number]: boolean } = {}
local mine: { [number]: boolean } = {}

-- The power-ups of each player (Game.hasBuff), its dash cooldown and its size, by the player's entity bits; and which
-- material (slot) shows each player's reload, with the last progress sent to it.
local buffs: { [number]: number } = {}
local dashLeft: { [number]: number } = {}
local sizes: { [number]: number } = {}
local slots: { [number]: number } = {}
local slotProgress: { [number]: number } = {}

-- Where each body was last sent, when, and how far the ball has turned, by entity bits.
local motions: { [number]: { position: Vec2, at: number } } = {}
local ballTurn: { [number]: number } = {}

--- Where to draw a body: its last position, moved along its velocity since it arrived.
local function smoothed(bits: number, position: Vec2, velocity: Vec2, now: number): Vec2
	local motion = motions[bits]
	if motion == nil or motion.position.x ~= position.x or motion.position.y ~= position.y then
		motion = { position = { x = position.x, y = position.y }, at = now }
		motions[bits] = motion
	end
	local lead = math.min(now - motion.at, maxLead)
	return { x = position.x + velocity.x * lead, y = position.y + velocity.y * lead }
end

local function sprite(texture: any, size: number, layer: number, alpha: number?): any
	return {
		texture = texture,
		source = { x = 0, y = 0, z = size, w = size },
		origin = { x = size / 2, y = size / 2 },
		color = { x = 1, y = 1, z = 1, w = alpha or 1 },
		layer = layer,
	}
end

-- Update: the match as the server sent it, and how long its state has lasted.
engine.ecs.system("game.TrackMatch", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Match" } },
	run = function(ctx)
		local match = State.match()
		match.since += ctx.time.delta
		for _, replicated in ctx.query do
			if replicated.state ~= match.state or replicated.overtime ~= match.overtime then
				match.since = 0
			end
			match.state = replicated.state
			match.timeLeft = replicated.timeLeft
			match.countdown = replicated.countdown
			match.red = replicated.red
			match.blue = replicated.blue
			match.overtime = replicated.overtime
			match.winner = replicated.winner
		end
	end,
})

-- Update: gives each new player its disc, and its name, kick ring and marker as children.
engine.ecs.system("game.DressPlayers", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Player" }, without = { "rtype.SpriteRenderer" } },
	run = function(ctx)
		for entity, player in ctx.query do
			ctx.commands:add(entity, "rtype.Transform", { position = player.position })
			ctx.commands:add(entity, "rtype.SpriteRenderer", sprite(assets.players[player.team], playerSize, playerDrawLayer))
			local ring = ctx.commands:spawn({
				["game.KickRing"] = { player = entity },
				["rtype.Transform"] = {},
				["rtype.SpriteRenderer"] = sprite(assets.kickRing, playerSize, ringDrawLayer, 0),
			})
			local tag = ctx.commands:spawn({
				["game.NameTag"] = { player = entity },
				["rtype.Transform"] = { position = { x = 0, y = nameOffset } },
				["rtype.TextRenderer"] = {
					text = player.name,
					font = assets.font,
					size = 13,
					color = { x = 1, y = 1, z = 1, w = 0.9 },
					anchor = { x = 0.5, y = 0 },
					layer = tagDrawLayer,
				},
			})
			local marker = ctx.commands:spawn({
				["game.Marker"] = { player = entity },
				["rtype.Transform"] = { position = { x = 0, y = -markerOffset } },
				["rtype.SpriteRenderer"] = {
					texture = assets.marker,
					source = { x = 0, y = 0, z = 16, w = 12 },
					origin = { x = 8, y = 6 },
					color = { x = 1, y = 1, z = 1, w = 0 },
					layer = tagDrawLayer,
				},
			})
			for kind = 1, 4 do
				local icon = ctx.commands:spawn({
					["game.BuffIcon"] = { player = entity, kind = kind },
					["rtype.Transform"] = { position = { x = 0, y = -iconOffset } },
					["rtype.SpriteRenderer"] = {
						texture = assets.powerups,
						source = { x = (kind - 1) * 32, y = 0, z = 32, w = 32 },
						origin = { x = 16, y = 16 },
						color = { x = 1, y = 1, z = 1, w = 0 },
						layer = iconDrawLayer,
					},
				})
				ctx.commands:setParent(icon, entity)
			end
			ctx.commands:setParent(ring, entity)
			ctx.commands:setParent(tag, entity)
			ctx.commands:setParent(marker, entity)
		end
	end,
})

-- Update: the players where the server says, in their team's color; which one is this client's; how many play.
engine.ecs.system("game.ShowPlayers", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Player", "net.Owner" }, write = { "rtype.Transform", "rtype.SpriteRenderer" } },
	calls = { "rtype.replication.localClient", "rtype.shader.setMaterialValue" },
	run = function(ctx)
		local local_ = ctx.call("rtype.replication.localClient", {})
		local me = State.me()
		me.connected, me.client = local_.connected, local_.client
		me.team = Game.spectators
		table.clear(names)
		table.clear(kicking)
		table.clear(mine)
		table.clear(buffs)
		table.clear(dashLeft)
		local red, blue = 0, 0
		local now = ctx.time.elapsed
		local seen = {}
		for entity, player, owner, transform, renderer in ctx.query do
			local bits = entity.bits
			seen[bits] = true
			transform.position = smoothed(bits, player.position, player.velocity, now)
			renderer.texture = assets.players[player.team]
			names[bits] = player.name
			kicking[bits] = player.kicking
			buffs[bits] = player.buffs
			dashLeft[bits] = player.dash

			-- Big players grow and shrink smoothly (their children follow the scale).
			local wanted = if Game.hasBuff(player.buffs, Game.big) then Game.bigFactor else 1
			local size = (sizes[bits] or 1) + (wanted - (sizes[bits] or 1)) * math.min(1, ctx.time.delta * 10)
			sizes[bits] = size
			transform.scale = { x = size, y = size }

			-- The dash's reload: a material of the player's own, swept as the cooldown runs out.
			local slot = slots[bits]
			if slot == nil then
				local used = {}
				for _, taken in slots do
					used[taken] = true
				end
				slot = 1
				while used[slot] do
					slot += 1
				end
				slot = math.min(slot, #assets.reloadMaterials)
				slots[bits] = slot
			end
			local material = assets.reloadMaterials[slot]
			if renderer.material ~= material then
				renderer.material = material
			end
			local progress = 1 - math.clamp(player.dash / Game.dashCooldown, 0, 1)
			if slotProgress[slot] ~= progress then
				slotProgress[slot] = progress
				ctx.call("rtype.shader.setMaterialValue", { material = material, name = "progress", x = progress, size = 1 })
			end
			if me.connected and owner.client == me.client then
				mine[bits] = true
				me.team = player.team
			end
			if player.team == Game.red then
				red += 1
			elseif player.team == Game.blue then
				blue += 1
			end
		end
		State.setTeamCounts(red, blue)
		for bits in slots do
			if not seen[bits] then
				slotProgress[slots[bits]] = nil
				slots[bits] = nil
				sizes[bits] = nil
			end
		end
	end,
})

-- Update: the power-ups a player has, above it: an icon each, side by side, the ones it has not hidden.
engine.ecs.system("game.UpdateBuffIcons", {
	phase = "Update",
	side = "client",
	query = { read = { "game.BuffIcon" }, write = { "rtype.SpriteRenderer", "rtype.Transform" } },
	after = { "game.ShowPlayers", "game.ShowBall", "game.UpdateKickRings", "game.UpdateMarkers" },
	run = function(ctx)
		local bob = math.sin(ctx.time.elapsed * 4) * 1.5
		for _, icon, renderer, transform in ctx.query do
			local mask = buffs[icon.player.bits] or 0
			local has = Game.hasBuff(mask, icon.kind)
			local alpha = if has then 1 else 0
			if renderer.color.w ~= alpha then
				renderer.color = { x = 1, y = 1, z = 1, w = alpha }
			end
			if has then
				-- The icons of the power-ups it has are centered above it, in the order of their kinds.
				local count, index = 0, 0
				for kind = 1, 4 do
					if Game.hasBuff(mask, kind) then
						count += 1
						if kind == icon.kind then
							index = count
						end
					end
				end
				transform.position = { x = (index - (count + 1) / 2) * iconStep, y = -iconOffset + bob }
				local scale = iconSize / 32
				transform.scale = { x = scale, y = scale }
			end
		end
	end,
})

-- Update: gives each power-up on the pitch its icon.
engine.ecs.system("game.DressPowerups", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Powerup" }, without = { "rtype.SpriteRenderer" } },
	run = function(ctx)
		for entity, powerup in ctx.query do
			ctx.commands:add(entity, "rtype.Transform", { position = powerup.position })
			ctx.commands:add(entity, "rtype.SpriteRenderer", {
				texture = assets.powerups,
				source = { x = (powerup.kind - 1) * 32, y = 0, z = 32, w = 32 },
				origin = { x = 16, y = 16 },
				color = { x = 1, y = 1, z = 1, w = 1 },
				layer = powerupDrawLayer,
			})
		end
	end,
})

-- Update: the power-ups on the pitch pulse so that they are seen.
engine.ecs.system("game.PulsePowerups", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Powerup" }, write = { "rtype.Transform" } },
	after = { "game.ShowPlayers", "game.ShowBall", "game.UpdateBuffIcons", "game.UpdateMarkers" },
	run = function(ctx)
		for _, powerup, transform in ctx.query do
			local pulse = 1 + math.sin(ctx.time.elapsed * 5 + powerup.position.x) * 0.12
			transform.scale = { x = pulse, y = pulse }
		end
	end,
})

-- Update: the names under the players.
engine.ecs.system("game.UpdateNameTags", {
	phase = "Update",
	side = "client",
	query = { read = { "game.NameTag" }, write = { "rtype.TextRenderer" } },
	after = { "game.ShowPlayers" },
	run = function(ctx)
		for _, tag, text in ctx.query do
			local name = names[tag.player.bits]
			if name ~= nil and text.text ~= name then
				text.text = name
			end
		end
	end,
})

-- Update: the white ring around the players holding kick.
engine.ecs.system("game.UpdateKickRings", {
	phase = "Update",
	side = "client",
	query = { read = { "game.KickRing" }, write = { "rtype.SpriteRenderer" } },
	after = { "game.ShowPlayers" },
	run = function(ctx)
		for _, ring, renderer in ctx.query do
			local alpha = if kicking[ring.player.bits] then 1 else 0
			renderer.color = { x = 1, y = 1, z = 1, w = alpha }
		end
	end,
})

-- Update: the marker bobbing over this client's player.
engine.ecs.system("game.UpdateMarkers", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Marker" }, write = { "rtype.SpriteRenderer", "rtype.Transform" } },
	after = { "game.ShowPlayers", "game.ShowBall", "game.UpdateKickRings" },
	run = function(ctx)
		local bob = math.sin(ctx.time.elapsed * 5) * 3
		for _, marker, renderer, transform in ctx.query do
			local alpha = if mine[marker.player.bits] then 1 else 0
			renderer.color = { x = 1, y = 1, z = 1, w = alpha }
			transform.position = { x = 0, y = -markerOffset + bob }
		end
	end,
})

-- Update: gives the ball its sprite.
engine.ecs.system("game.DressBall", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ball" }, without = { "rtype.SpriteRenderer" } },
	run = function(ctx)
		for entity, ball in ctx.query do
			ctx.commands:add(entity, "rtype.Transform", { position = ball.position })
			ctx.commands:add(entity, "rtype.SpriteRenderer", sprite(assets.ball, ballSize, ballDrawLayer))
		end
	end,
})

-- Update: the ball where the server says, turning as it rolls (one turn per circumference travelled).
engine.ecs.system("game.ShowBall", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ball" }, write = { "rtype.Transform" } },
	after = { "game.ShowPlayers" },
	run = function(ctx)
		local now, dt = ctx.time.elapsed, ctx.time.delta
		for entity, ball, transform in ctx.query do
			local bits = entity.bits
			transform.position = smoothed(bits, ball.position, ball.velocity, now)
			local roll = Game.length(ball.velocity) * dt / Game.ballRadius
			local turn = (ballTurn[bits] or 0) + (if ball.velocity.x >= 0 then roll else -roll)
			ballTurn[bits] = turn % (2 * math.pi)
			transform.rotation = ballTurn[bits]
		end
	end,
})
