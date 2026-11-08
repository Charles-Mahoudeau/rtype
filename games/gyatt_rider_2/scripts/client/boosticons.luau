-- The boost icons, when the settings ask for them: over each cyclist that carries a boost, the image of its effect
-- (assets/particles/), so that a player who does not tell the colors apart knows which boost it is.

local Game = require("@scripts/shared/rules")
local Settings = require("@scripts/client/settings")

local function texture(path: string)
	return engine.assets.texture(path, { linear = false })
end

local iconSize = 44 -- world units
local iconLift = 84 -- world units above the cyclist's middle
local iconLayer = 3 -- over the riders
local icons = {
	[Game.pointsBoost] = { texture = texture("particles/twinkle.png"), color = { x = 1, y = 1, z = 1, w = 1 } },
	[Game.speedBoost] = { texture = texture("particles/boost.png"), color = { x = 1, y = 1, z = 1, w = 1 } },
	[Game.shieldBoost] = { texture = texture("particles/shield.png"), color = { x = 0.4, y = 1, z = 0.4, w = 1 } },
	[Game.sniperBoost] = { texture = texture("particles/scope.png"), color = { x = 1, y = 1, z = 1, w = 1 } },
}
local imageSide = 128 -- pixels of each particle image

-- This frame's boosted cyclists, by their bits: where each is; and the cyclists that have an icon.
local cyclistAt: { [number]: { x: number, y: number } } = {}
local iconed: { [number]: boolean } = {}

-- Update, once everything moved: where each boosted cyclist is, and an icon for each new one while the settings ask.
engine.ecs.system("game.PlaceBoostIcons", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Cyclist", "rtype.Transform" } },
	after = { "game.MoveBoostFx" },
	run = function(ctx)
		table.clear(cyclistAt)
		local shown = Settings.get("boostIcons")
		for entity, cyclist, transform in ctx.query do
			local icon = icons[cyclist.boost]
			if icon == nil then
				continue
			end
			cyclistAt[entity.bits] = transform.position
			if shown and not iconed[entity.bits] then
				iconed[entity.bits] = true
				local scale = iconSize / imageSide
				ctx.commands:spawn({
					["game.BoostIcon"] = { cyclist = entity },
					["rtype.Transform"] = { position = transform.position, scale = { x = scale, y = scale } },
					["rtype.SpriteRenderer"] = {
						texture = icon.texture,
						source = { x = 0, y = 0, z = imageSide, w = imageSide },
						origin = { x = imageSide / 2, y = imageSide / 2 },
						color = icon.color,
						layer = iconLayer,
					},
				})
			end
		end
	end,
})

-- Update: each icon floats over its cyclist; it goes with it, or when the settings no longer ask for icons.
engine.ecs.system("game.MoveBoostIcons", {
	phase = "Update",
	side = "client",
	query = { read = { "game.BoostIcon" }, write = { "rtype.Transform" } },
	after = { "game.PlaceBoostIcons" },
	run = function(ctx)
		local shown = Settings.get("boostIcons")
		for entity, icon, transform in ctx.query do
			local at = cyclistAt[icon.cyclist.bits]
			if at == nil or not shown then
				iconed[icon.cyclist.bits] = nil
				ctx.commands:despawn(entity)
				continue
			end
			transform.position = { x = at.x, y = at.y - iconLift }
		end
	end,
})
