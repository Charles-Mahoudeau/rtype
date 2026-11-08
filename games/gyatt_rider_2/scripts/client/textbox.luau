-- The game's text box (assets/text_box.png, a slanted light panel): every big text of the interface sits on one, tinted
-- in the color of what it shows (a player's, the menu's...), its text in ink.

local assets = require("@scripts/client/assets")

local ink = { 0.08, 0.08, 0.1, 1 }

return {
	--- The ink of the texts on a text box: { r, g, b, a }.
	ink = ink,
	--- A UI spec: a text box laid out by @p style (a style or a list of styles), tinted @p color ({ r, g, b, a }),
	--- holding @p children centred in it. @p extra adds keys to the spec (`button`, `components`...).
	node = function(style, color, children, extra)
		local spec = {
			style = { style, { justify = "center", alignItems = "center" } },
			image = { texture = assets.textBox, color = color },
			children = children,
		}
		for key, value in extra or {} do
			spec[key] = value
		end
		return spec
	end,
	--- A UI spec of a line of text in ink, @p size high, that lets the pointer through to its box.
	text = function(text: string, size: number)
		return { style = { ignorePointer = true }, text = { text = text, font = assets.font, size = size, color = ink } }
	end,
}
