-- The client's assets and colors. The forms load the assets on the clients only.

-- The engine's colors are linear: a color picked on screen (sRGB) is turned to linear before use.
local function linear(channel: number): number
	return if channel <= 0.04045 then channel / 12.92 else ((channel + 0.055) / 1.055) ^ 2.4
end

--- An sRGB color such as 0xE53935, as the `{ r, g, b, a }` the UI and particle forms take.
local function rgb(hex: number, alpha: number?): { number }
	local r, g, b = bit32.extract(hex, 16, 8), bit32.extract(hex, 8, 8), bit32.extract(hex, 0, 8)
	return { linear(r / 255), linear(g / 255), linear(b / 255), alpha or 1 }
end

--- The same color as the vec4 a component's field holds.
local function vec4(hex: number, alpha: number?): { x: number, y: number, z: number, w: number }
	local color = rgb(hex, alpha)
	return { x = color[1], y = color[2], z = color[3], w = color[4] }
end

return {
	rgb = rgb,
	vec4 = vec4,
	-- The teams' colors, by team.
	teamColors = { [1] = 0xE53935, [2] = 0x2F74E8 },
	gold = 0xFFD54F,

	pitch = engine.assets.texture("sprites/pitch.png"),
	players = {
		[1] = engine.assets.texture("sprites/player_red.png"),
		[2] = engine.assets.texture("sprites/player_blue.png"),
	},
	ball = engine.assets.texture("sprites/ball.png"),
	kickRing = engine.assets.texture("sprites/kick_ring.png"),
	marker = engine.assets.texture("sprites/marker.png"),
	font = engine.assets.font("fonts/orbitron.json"),

	sounds = {
		kick = engine.assets.sound("sounds/kick.wav"),
		bounce = engine.assets.sound("sounds/bounce.wav"),
		whistle = engine.assets.sound("sounds/whistle.wav"),
		whistleLong = engine.assets.sound("sounds/whistle_long.wav"),
		beep = engine.assets.sound("sounds/beep.wav"),
		cheer = engine.assets.sound("sounds/cheer.wav"),
		click = engine.assets.sound("sounds/click.wav"),
	},

	-- The particles' textures (effects.luau makes the effects).
	dot = engine.assets.texture("particles/dot.png"),
	smoke = engine.assets.texture("particles/smoke.png"),
	confetti = engine.assets.texture("particles/confetti.png"),
	ring = engine.assets.texture("particles/ring.png"),
	star = engine.assets.texture("particles/star.png"),
}
