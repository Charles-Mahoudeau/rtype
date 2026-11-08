# Accessibility

What the engine offers for players with disabilities, what a game must do with it, and what the engine still lacks.
The game developer's side is the manual page [Accessibility](https://charles-mahoudeau.github.io/rtype/accessibility/); this page is for engine
developers, to know where the gaps are.

## By category

| Category | The engine provides | The game does | Missing in the engine |
|---|---|---|---|
| **Motor** | Several bindings per action, keyboard, mouse and gamepad at once, the largest value wins ([`Input.md`](Input.md)); held states read every frame (no timing needed); gamepad deadzones | Binds each action to several controls; designs for "hold" rather than "tap"; offers an easy mode | Runtime remapping (no call removes a binding, bindings are not saved per player); hold-to-toggle; more than one gamepad per client |
| **Visual** | Text from signed-distance fonts, sharp at any size ([`Text.md`](Text.md)); a virtual resolution scaled to the window, resizable and fullscreen windows; user shaders ([`Shaders.md`](Shaders.md)) | Distinguishes players and bullets by shape, not only colour; uses large text | A colour-blind filter as a full-screen pass; a global UI scale setting; high-contrast theme |
| **Hearing** | Separate `music` and `effects` buses with their own volumes ([`audio/design.md`](audio/design.md)); stereo panning from the sound's position | Shows a visual cue for every important sound | Changing bus volumes at run time from a script; a subtitle or caption helper |
| **Cognitive** | Fixed timestep, so a game can lower its speeds; data-driven settings for an easy mode | Clear goals, a pause, consistent controls, difficulty options | – |

## The documentation

The two documentation sites (this one and the manual) are built with Zensical and its Material theme:

- every page has a heading structure, a table of contents and full-text search;
- the sites are navigable with the keyboard alone and readable by screen readers (semantic HTML);
- the theme follows the system's light or dark preference;
- code blocks are text, not images, and can be copied.

Diagrams are Mermaid source, rendered as SVG, and every diagram is explained in the text around it.

## Next steps

In order of value for the cost:

1. **Runtime remapping**: `rtype.input.unbind` and a way to save bindings with the persistence plugin, so a game can
   offer a controls menu.
2. **Volumes at run time**: an engine call that sets `AudioVolumes`, for an options menu.
3. **Colour-blind filter**: a full-screen pass in the renderer with the three common simulations/corrections
   (protanopia, deuteranopia, tritanopia), chosen in the settings.
4. **UI scale**: one factor applied to every UI node and text, from the settings.
