-- The splash screen's zoom: the image grows fast, then settles at its full size.
local ZOOM_SECONDS = 4
local LONGEST_STEP = 1 / 30 -- A slow frame (the image being loaded) must not skip the zoom.
local START = 0.6 -- Share of the full size it starts from.

engine.ecs.system("game.zoom", {
	phase = "Update",
	query = { write = { "game.Splash" } },
	run = function(ctx)
		for _, splash in ctx.query do
			splash.elapsed += math.min(ctx.delta, LONGEST_STEP) * splash.pace
			local progress = math.min(splash.elapsed / ZOOM_SECONDS, 1)
			local eased = 1 - (1 - progress) ^ 3
			splash.zoom = START + (1 - START) * eased
			splash.finished = progress >= 1
		end
	end,
})
