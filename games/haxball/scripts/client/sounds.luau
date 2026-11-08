-- The sounds: kicks and bounces where they happen, louder the harder, the crowd and the whistle on a goal, the beeps
-- of the countdown, the whistle at each kickoff and at the end, and a click on the menu's buttons.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/client/state")
local assets = require("@scripts/client/assets")

local hardestKick = 800 -- the ball's speed after a kick, in world units per second, that sounds the loudest
local hardestBounce = 600

-- What the last frame saw of the match, to hear when it changes.
local heard = { state = Game.waiting, second = 0, overtime = false }

-- Update: plays what the server told and what changed in the match.
engine.ecs.system("game.PlaySounds", {
	phase = "Update",
	side = "client",
	query = {},
	events = { read = { "net.PacketReceived", "rtype.UiClicked" } },
	calls = { "rtype.audio.playSound" },
	after = { "game.TrackMatch" },
	run = function(ctx)
		local function play(sound: any, volume: number, position: { x: number, y: number }?, pitch: number?)
			ctx.call("rtype.audio.playSound", { sound = sound, volume = volume, pitch = pitch or 1, position = position })
		end
		for _, packet in engine.net.received(ctx) do
			local values = packet.values
			local at = { x = values.x, y = values.y }
			if packet.name == "game.Kicked" then
				local strength = math.clamp(values.power / hardestKick, 0.3, 1)
				play(assets.sounds.kick, strength, at, math.lerp(0.95, 1.08, math.random()))
			elseif packet.name == "game.Bounce" then
				play(assets.sounds.bounce, math.clamp(values.speed / hardestBounce, 0.15, 0.8), at, math.lerp(0.9, 1.1, math.random()))
			elseif packet.name == "game.Goal" then
				play(assets.sounds.cheer, 0.9)
				play(assets.sounds.whistle, 0.7)
			end
		end
		for _ in ctx.events.read("rtype.UiClicked") do
			play(assets.sounds.click, 0.6)
		end

		local match = State.match()
		local second = math.ceil(match.countdown)
		if match.state == Game.kickoff and second ~= heard.second and second > 0 then
			play(assets.sounds.beep, 0.5)
		end
		if match.state ~= heard.state then
			if match.state == Game.playing then
				play(assets.sounds.whistle, 0.6)
			elseif match.state == Game.ended then
				play(assets.sounds.whistleLong, 0.7)
			end
		elseif match.overtime and not heard.overtime then
			play(assets.sounds.whistle, 0.6)
		end
		heard.state, heard.second, heard.overtime = match.state, second, match.overtime
	end,
})
