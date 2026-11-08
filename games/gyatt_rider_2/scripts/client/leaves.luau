-- A player who leaves is announced: "Player 2 left", on a text box of its color under the scores, for a few seconds. A
-- client sees it from the riders the server replicates: a slot that had a rider and has none any more.

local Hud = require("@scripts/client/hud")
local TextBox = require("@scripts/client/textbox")

local shownSeconds = 4

-- The column the notices stack in, and the slots that had a rider last frame.
local column = nil
local ridden: { [number]: boolean } = {}

-- Startup: the column, centred under the scores.
engine.ecs.system("game.SpawnLeaveNotices", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		column = engine.ui.spawn(ctx.commands, {
			style = {
				width = "100%",
				height = "100%",
				alignItems = "center",
				paddingTop = 64,
				gap = 8,
				ignorePointer = true,
			},
		})
	end,
})

-- Update: a notice for each slot whose rider is gone since last frame.
engine.ecs.system("game.NoticeLeaves", {
	phase = "Update",
	side = "client",
	query = {},
	after = { "game.ReadRiders" },
	run = function(ctx)
		local now = Hud.pointsOf()
		for slot in ridden do
			if now[slot] == nil and column ~= nil then
				engine.ui.spawn(
					ctx.commands,
					TextBox.node(
						{ width = 300, height = 60, ignorePointer = true },
						Hud.colorOf(slot),
						{ TextBox.text(`Player {slot + 1} left`, 24) },
						{ components = { ["game.LeaveNotice"] = { remaining = shownSeconds } } }
					),
					column
				)
			end
		end
		table.clear(ridden)
		for slot in now do
			ridden[slot] = true
		end
	end,
})

-- Update: a notice goes once it has been shown long enough.
engine.ecs.system("game.FadeLeaveNotices", {
	phase = "Update",
	side = "client",
	query = { write = { "game.LeaveNotice" } },
	run = function(ctx)
		for entity, notice in ctx.query do
			notice.remaining -= ctx.time.delta
			if notice.remaining <= 0 then
				ctx.commands:despawn(entity)
			end
		end
	end,
})
