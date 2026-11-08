-- Closing the window quits: the engine does not stop the game by itself when the window closes. Escape opens the menu
-- (client/menu.luau), which can quit too.

print("ZQSD (WASD on QWERTY) to move, the arrows to fire, F3 for the colliders, Escape for the menu.")

-- Update: the window's close button closes the game.
engine.ecs.system("game.Quit", {
	phase = "Update",
	side = "client",
	query = {},
	events = { read = { "rtype.Closed" }, send = { "rtype.AppExit" } },
	run = function(ctx)
		for _ in ctx.events.read("rtype.Closed") do
			ctx.events.send("rtype.AppExit", { code = 0 })
			return
		end
	end,
})
