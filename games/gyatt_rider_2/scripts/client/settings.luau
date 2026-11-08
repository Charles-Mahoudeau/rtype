-- The player's settings, chosen in the menu (client/menu.luau) and kept between games in a save of the client
-- (rtype.Persistence, "settings"). The modules read them each frame, so a change shows at once.

local defaults = {
	effectsVolume = 1, -- of the shots, deaths and kills, 0 to 1
	reduceMotion = false, -- no screen shake; ghosts fade instead of flashing
	boostIcons = false, -- an icon over each cyclist carrying a boost, not only its color
}
local saveName = "settings"

local values = table.clone(defaults)
local unsaved = false

-- At load: the saved settings over the defaults. A missing or damaged save keeps the defaults; a field of another
-- type, or unknown, is skipped.
do
	local loaded = engine.persistence.read(nil, { name = saveName })
	local saved = if loaded.ok then engine.json.tryDecode(loaded.data) else nil
	if type(saved) == "table" then
		for name, default in defaults do
			if type(saved[name]) == type(default) then
				values[name] = saved[name]
			end
		end
	end
end

-- Update: writes the settings once they changed.
engine.ecs.system("game.SaveSettings", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "rtype.json.encode", "rtype.persistence.write" },
	run = function(ctx)
		if not unsaved then
			return
		end
		unsaved = false
		local written = engine.persistence.write(ctx, { name = saveName, data = engine.json.encode(values) })
		if not written.ok then
			print(`[client] the settings were not saved: {written.error}`)
		end
	end,
})

return {
	--- The setting @p name.
	get = function(name: string): any
		return values[name]
	end,
	--- Changes the setting @p name; it is saved at the end of the frame.
	set = function(name: string, value: any)
		assert(defaults[name] ~= nil, `unknown setting '{name}'`)
		if values[name] ~= value then
			values[name] = value
			unsaved = true
		end
	end,
}
