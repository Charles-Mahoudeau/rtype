# Text

The text plugin (`src/rtype/plugins/text/`, `rtype::plugins::text`) draws text from **MSDF font atlases**, so it stays
sharp from 12 to 200 pixels and beyond, rotated or scaled, from one small image per font. It lays the text out itself
and hands the renderer the glyphs already placed (`IRenderer::draw(Text)`), which Vulkan batches with the sprites. The
`BackendPlugin` adds it.

## Drawing a text

```cpp
void spawnTitle(ecs::ResMut<FontAssets> fonts, ecs::Commands& commands) {
    commands.spawn()
        .add(Transform{.position = {400.0F, 300.0F}})
        .add(TextRenderer{.text = "Player 1 wins!", .font = fonts->load("assets/fonts/orbitron.json"),
                          .size = 48.0F, .anchor = {0.5F, 0.5F}, .layer = 10});
}
```

- **`FontAssets`** (the resource `rtype.FontAssets`) keeps the fonts by `AssetId`, as `TextureAssets` keeps the
  textures. `load()` reads a font's metrics at once, on every side (a server can measure texts); one that cannot be read
  is recorded as failed, with the reason, which is printed. Its atlas image is handed to `TextureAssets` by the client,
  with linear filtering and `TextureEncoding::kLinear`.
- **`TextRenderer`** (the component `rtype.TextRenderer`) is what to write: the UTF-8 text (`\n` starts a line), the
  font, its size (world units per em, before the entity's scale), its colour, how its lines line up (`TextAlign`), its
  anchor (which point of the text sits on the entity's position, and turns: `{0, 0}` the top-left corner, `{0.5, 0.5}`
  the centre), its layer among the sprites and its blend mode. Where it is drawn comes from the entity's
  `GlobalTransform`, as for a `SpriteRenderer`.
- **`layoutText()`** and **`measureText()`** place a text glyph by glyph, or only measure it, without the ECS: to
  centre a text, align it to a corner or size a panel.

## Layout

`layoutText(font, text, size, align)` decodes the UTF-8 text (an invalid byte becomes U+FFFD) and moves a pen along
each line: each glyph is placed by its plane bounds from the pen on the baseline, and the pen moves by its advance plus
the kerning of the pair. The first baseline sits the font's ascender below the top, the next ones one line height
further. A character the font does not have is drawn as `?`, or skipped if the font has no `?` either; spaces move
the pen without a glyph. The text's size is the width of its widest line by the height of its lines, which centred and
right-aligned lines are placed in.

## Drawing

Render "rtype.Text.Draw", on the client, between the backend's camera and its sprites, lays out every `TextRenderer`
whose font and atlas are loaded and pushes a `graphics::Text` to the `SpriteQueue`: the `BackendPlugin` sorts it with
the sprites by layer and calls `IRenderer::draw(Text)`.

In Vulkan, each glyph is one instance of the sprite batch, so a text is drawn by the same instanced draws as the
sprites of its atlas, in their order. Its instance carries a flag and the atlas' distance range in its blend word
(`rendering/Blend.hpp`), and `sprite.frag` reads its texture as three distances: their median, minus one half, is the
signed distance to the edge, scaled from atlas pixels to screen pixels (`fwidth`) so the edge is about one screen pixel
wide at any size. The atlas is created `R8G8B8A8_UNORM`: an sRGB image would bend the distances.

## Making a font

Fonts are generated offline with [msdf-atlas-gen](https://github.com/Chlumsky/msdf-atlas-gen) (`brew install
msdf-atlas-gen` on macOS, or a release binary), from any `.ttf` or `.otf`. `assets/fonts/orbitron.png` and `.json`
were made from `Orbitron.ttf` ([Orbitron](https://github.com/theleagueof/orbitron), SIL Open Font License,
`assets/fonts/OFL.txt`), for ASCII and Latin-1 (`assets/fonts/charset.txt`):

```sh
cd assets/fonts
msdf-atlas-gen -font Orbitron.ttf -charset charset.txt -type msdf -format png -imageout orbitron.png \
    -json orbitron.json -size 48 -pxrange 4 -yorigin top -pots
```

- `-type msdf` (or `mtsdf`): the other types are rejected.
- `-imageout <name>.png -json <name>.json`: the JSON does not name its image, the plugin takes the same path with
  `.png`.
- `-size` is the em size in atlas pixels: 48 keeps the corners sharp at any size; `-pxrange 4` the width of the
  distance field. Larger fonts need neither more.
- `-yorigin top` is not required: a file written Y up is flipped.

Orbitron has no kerning table msdf-atlas-gen can read, so its JSON has no `kerning`; fonts that have one get it.

## Example

`examples/plugins/text` writes a line at every size from 12 to 200 pixels, a text that zooms and turns and a centred
block of several lines. `--validation` enables the Vulkan validation layer.

```sh
xmake f --text=y && xmake run text --validation
```
