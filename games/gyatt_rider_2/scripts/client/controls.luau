-- Escape, or closing the window, quits: the engine does not stop the game by itself when the window closes. The quit
-- action is the client's own: added by a call rather than by engine.input.action, it stays out of the inputs sent to
-- the server.

print("ZQSD (WASD on QWERTY) to move, the arrows to fire, F3 for the colliders, Escape to quit.")

engine.call("rtype.input.addAction", { name = "quit", type = "button" })
engine.call("rtype.input.bind", { action = "quit", control = "Escape" })

-- Update: Escape, or the window's close button, closes the game.
engine.ecs.system("game.Quit", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "rtype.input.read" },
	events = { read = { "rtype.Closed" }, send = { "rtype.AppExit" } },
	run = function(ctx)
		local closed = false
		for _ in ctx.events.read("rtype.Closed") do
			closed = true
		end
		if closed or ctx.call("rtype.input.read", { action = "quit" }).pressed then
			ctx.events.send("rtype.AppExit", { code = 0 })
		end
	end,
})
