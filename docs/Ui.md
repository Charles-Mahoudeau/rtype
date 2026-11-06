# User interface

The UI is ECS-native: a node is an entity, its children are its ECS children, and [Yoga](https://www.yogalayout.dev/) lays
them out with flexbox. It is drawn over everything else, in view pixels. List `"rtype.Ui"` in `engine.settings.plugins`;
`examples/plugins/ui` is a complete menu, in Luau only.

Nothing has a fixed look. There is no `Button` entity to subclass: a button is any node with a `UiButton`, whatever else
it holds. A card with an image, a title and a button is three nodes, and a health bar is a node inside a node.

## Writing an interface

`engine.ui.spawn(commands, table)` spawns a tree. Every key but `style` and `children` adds a component.

```lua
local font = engine.assets.font("fonts/orbitron.json")
local row = { direction = "row", alignItems = "center", gap = 12 }  -- a style is a plain table

engine.ecs.system("game.Menu", {
    phase = "Startup",
    side = "client",
    query = {},
    run = function(ctx)
        engine.ui.spawn(ctx.commands, {
            style = { width = "100%", height = "100%", justify = "center", alignItems = "center" },
            children = {
                {
                    style = { row, { padding = 16 } },  -- a list of styles is merged, the last one winning
                    background = { color = { 0.1, 0.1, 0.2, 1 }, radius = 8 },
                    button = { action = "menu.play", hovered = { 0.2, 0.2, 0.4, 1 } },
                    children = { { text = { text = "Play", font = font, size = 24 } } },
                },
            },
        })
    end,
})
```

Components can also be spawned by hand, in Luau or C++: `engine.ui.spawn` only fills them. It takes `components` for the
game's own, and returns the entity.

| Key | Component | Fields |
|---|---|---|
| `style` | `UiNode` | see below |
| `background` | `UiBackground` | `color`, `radius`, `borderColor`, `borderWidth` |
| `image` | `UiImage` | `texture`, `source`, `color`, `slice` (a number, or left, top, right, bottom), `sliceScale` |
| `text` | `UiText` | `text`, `font`, `size`, `color`, `align`, `wrap` |
| `button` | `UiButton` | `action`, `normal`, `hovered`, `pressed`, `disabled` |
| `toggle` | `UiToggle` | `action`, `checked`, `mark`, `inset` |
| `slider` | `UiSlider` | `action`, `min`, `max`, `value`, `step`, `track`, `fill`, `handle`, `trackHeight`, `handleRadius` |
| `input` | `UiTextInput` | `action`, `placeholder`, `maxLength`, `caret` |
| `scroll` | `UiScroll` | `speed` |
| `interaction` | `UiInteraction` | `disabled`, `focusable` |

Colours are `{ r, g, b, a }`, linear like every colour of the engine: on an sRGB window, 0.25 looks brighter than the
hexadecimal `#404040`.

## Layout

`UiNode` holds every flexbox property of Yoga. Lengths are a number of pixels, `"50%"` or `"auto"`.

| Style | Values |
|---|---|
| `direction` | `column` (default), `column-reverse`, `row`, `row-reverse` |
| `justify` | `start`, `center`, `end`, `space-between`, `space-around`, `space-evenly` |
| `alignItems`, `alignSelf`, `alignContent` | `start`, `center`, `end`, `stretch`, `baseline`, and for `alignContent` the `space-*` ones |
| `wrap` | `nowrap`, `wrap`, `wrap-reverse` |
| `grow`, `shrink`, `basis` | numbers, and a length for `basis` |
| `width`, `height`, `minWidth`, `minHeight`, `maxWidth`, `maxHeight`, `aspectRatio` | lengths, and a number for `aspectRatio` |
| `position`, `left`, `top`, `right`, `bottom` | `relative`, `absolute`, and lengths |
| `margin`, `marginX`, `marginY`, `marginLeft`... and `padding`, the same | lengths |
| `gap`, `rowGap`, `columnGap`, `border` | pixels |
| `overflow` | `visible`, `hidden`, `scroll` |
| `display` | `flex`, `none` |
| `zIndex`, `ignorePointer` | draw order among siblings, and letting the pointer through |

There are no anchors: an absolute node with insets is one. `{ position = "absolute", right = 16, top = 16 }` stays in the
top-right corner of its parent, whatever the size of the view.

A node without a parent node is a root, laid out over the whole view. Where a node ended up is in its `UiComputed`, for the
game to read. The tree is built again each frame: it is simple and has no stale state, and hundreds of nodes cost little.
Should a screen with thousands of nodes need it, keeping the Yoga nodes between frames is the way.

## Reacting

A widget says what it means with its `action`, the game's own name: one reader serves a whole menu. The UI runs in
PreUpdate, so a system in Update reads what happened in the same frame.

```lua
engine.ecs.system("game.ReadMenu", {
    phase = "Update",
    side = "client",
    query = {},
    events = { read = { "rtype.UiClicked", "rtype.UiValueChanged" } },
    run = function(ctx)
        for click in ctx.events.read("rtype.UiClicked") do
            if click.action == "menu.play" then
                -- ...
            end
        end
    end,
})
```

| Event | Sent by | Fields |
|---|---|---|
| `UiClicked` | `UiButton` | `entity`, `action` |
| `UiValueChanged` | `UiSlider`, `UiToggle` (0 or 1) | `entity`, `action`, `value` |
| `UiTextChanged` | `UiTextInput` | `entity`, `action`; the text is in its `UiText` |
| `UiTextSubmitted` | `UiTextInput`, on Enter | `entity`, `action` |

`UiInteraction` holds the state of a node (`hovered`, `pressed`, `focused`, `clicked`), for what the widgets do not cover:
any node can have one and be read by a system.

The UI blocks nothing by itself: gameplay reads `UiPointer::overUi` to ignore clicks on the UI, and `UiFocus::typing` to
ignore the keyboard while a field is being typed in.

## Focus

The pointer, the keyboard and a gamepad all work on every widget.

| Input | Effect |
|---|---|
| Tab, Shift+Tab | The next or previous widget, in draw order |
| Arrows, directional pad | The nearest widget that way; left and right adjust a slider |
| Enter, Space, South button | Activates the focused widget |
| Escape, East button | Drops the focus |

## How it is drawn

`rtype.ui.draw` pushes the frame's nodes to the `OverlayQueue` of the graphics plugin, and never touches the backend, so it
needs no order with the plugins that draw. The queue goes to `IRenderer::drawOverlay()`: unlike the other draws, its items
are in view pixels, stay in the order given, sprites, shapes and texts mixed, and each has its clip. Vulkan records them
after the frame, in runs of one kind and one clip, each a scissored draw, with its own view. Any other plugin can push to
the queue too. `ViewInfo` gives the sizes of the frame.

## Limits

- The UI is on screen only: no UI in the world, on a render target or in perspective.
- `UiTextInput` is a single line, without selection, clipboard or scrolling: a text wider than the field overflows it.
- `UiSlider` is horizontal, and `UiScroll` has no scroll bar.
- A `UiText` that wraps breaks between words only.
