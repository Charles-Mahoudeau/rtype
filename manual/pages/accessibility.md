# Accessibility

Players with a motor, visual, hearing or cognitive disability should be able to play your game. This page lists what
the engine gives you for it today, how to use it, and what it does not do yet, so you can plan around it.

## Motor: let players choose how they play

**Bind every action to several controls.** An action can have any number of bindings, from the keyboard, the mouse and
a gamepad at once. The one pushed the furthest wins, so they never add up. Offer at least the arrows, a second key set
and a gamepad:

```luau
engine.input.action("move", "vector2", {
	bind = {
		{ up = "Up", down = "Down", left = "Left", right = "Right" },
		{ up = "W", down = "S", left = "A", right = "D" },
		{ x = "GamepadLeftX", y = "GamepadLeftY" },
	},
})
engine.input.action("shoot", "button", { bind = { "Space", "MouseLeft", "GamepadSouth" } })
```

A player who can use one hand, or only a gamepad, can then play without any setting.

**Read held states, not presses.** Your systems read `held` from `rtype.input.read`, so a player who cannot press
quickly can hold the button down. Prefer "hold to fire" to "tap to fire" in your design, and avoid actions that need
two buttons at once.

**Let them slow down.** Your gameplay runs on a fixed timestep. Put the speeds of your enemies and bullets in a
resource or a setting, so a game option can lower them for an easier mode.

## Visual: never rely on colour alone

- **Distinguish players by more than colour**: a different sprite, a shape or a label next to each ship. The four
  players must stay distinct for a colour-blind player.
- **Make danger readable**: enemy bullets should differ from yours by shape and size, not only hue.
- **Keep text large.** Text is drawn from signed-distance fonts and stays sharp at any `size`
  ([User interface](ui.md)): use 24 or more for gameplay information.
- **Scale with the window.** `engine.settings.vulkan.virtualResolution` makes your game designed for one resolution
  and scaled to the window, so a player can make the window larger, or set `window.fullscreen`
  ([Project settings](project-settings.md)).

## Hearing: show what you play

- **Every important sound has a visual cue.** When a sound warns of something (a boss arrives, a shot hits the
  player), show it too: a flash, an icon, a text line.
- **Separate music from effects.** Play music on the `"music"` bus and effects on the `"effects"` bus
  (`rtype.audio.playSound`, argument `bus`), and set their starting levels in `project.luau`:

```luau
engine.settings.audio.volume.music = 0.6
engine.settings.audio.volume.effects = 1.0
```

## Cognitive: keep it clear

- One goal at a time, said in a short sentence on screen.
- A pause the player controls (an action bound to `Escape` and a gamepad button).
- Consistent controls and colours from one level to the next.
- An easy mode: fewer enemies, slower bullets, more lives. Keep its values in data so you can tune them.

## What the engine does not do yet

| Missing | What you can do now |
|---|---|
| Remapping keys from a menu (there is no call to remove a binding) | Offer several bindings per action (above) |
| Changing the music and effect volumes while the game runs | Set them in `project.luau`; lower `volume` in your `playSound` calls |
| Colour-blind filters on the whole screen | Choose a palette safe for colour blindness, and shapes as well as colours |
| Subtitles | Show a text line with your `ui` for each important sound |
| More than one gamepad | One gamepad per client; in network play, each player has their own client |

## See also

- [Project settings](project-settings.md): window, audio volumes, virtual resolution.
- [Engine calls](engine-calls.md): `rtype.input.*` and `rtype.audio.*`.
- [User interface](ui.md): text, buttons, layout.
