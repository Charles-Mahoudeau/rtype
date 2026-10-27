# Sprites: atlases and animations

The sprite plugin (`src/rtype/plugins/sprite/`, `rtype::plugins::sprite`) cuts textures into named frames and plays
animations made of them. It knows nothing of the renderer: it writes the `SpriteRenderer` of each animated entity, which
the backend plugin draws as before. The `BackendPlugin` adds it; a server without a window adds `SpritePlugin` itself if
it wants its animators to advance.

## Atlases

A `SpriteAtlas` is a texture, its frames (a name, a rectangle of the texture, an origin) and its animations. The
`AtlasAssets` resource keeps them by `AssetId`, as `TextureAssets` keeps textures, and builds them three ways:

```cpp
// An Aseprite or TexturePacker JSON file: its image, frames and animations (Aseprite's frame tags).
const AssetId ship = atlases->load("assets/ship.json");

// A uniform sheet without a file: a grid, and animations in code.
SpriteAtlas enemy = SpriteAtlas::grid("assets/r-typesheet26.gif", {63.0F, 48.0F}, 6, {2.0F, 0.0F}, 0, {1.0F, 1.0F});
enemy.addAnimation("fly", {0, 1, 2, 3, 4, 5}, 0.08F);
const AssetId enemies = atlases->add("enemy", std::move(enemy));

// Separate images packed into one texture at load time: one frame per image, named after the file's stem.
const std::array images{std::filesystem::path{"assets/spark.png"}, std::filesystem::path{"assets/smoke.png"}};
const AssetId fx = atlases->pack("fx", images);
```

- **The atlas is built at once**, on every side: a server gets the same frames and animations, and an error is reported
  where the atlas is asked for. One that cannot be built is recorded as failed, with the reason, which is printed: a
  missing atlas does not stop the game. Its **texture** is handed to `TextureAssets` by the client's
  `rtype.Sprite.Textures` system and loaded before the frame that draws it, like any texture.
- **The JSON file** is what Aseprite (File > Export Sprite Sheet, JSON Data) and TexturePacker export, in their "Hash"
  or "Array" layout. Frames keep the file's order. Each frame's `duration` (milliseconds) is read, and a trimmed frame
  keeps its origin where it was in the untrimmed image. Each frame tag becomes an animation, with its `direction`
  (forward, reverse, pingpong, pingpong_reverse) and its `repeat` count. `meta.image` is relative to the file. The
  parser lives in `sprite/detail/AtlasJson`, the only file that includes nlohmann_json.
- **Packing** decodes the images (stb_image, through `backend/graphics/ImageDecoding`), packs them with stb_rect_pack
  into the smallest power-of-two texture up to 4096x4096, a pixel apart by default so linear filtering never bleeds,
  and hands the pixels to `TextureAssets::loadPixels()`.
- **A frame's origin** (the point the sprite is placed and turned around) is its centre, unless the file gives a
  TexturePacker `pivot`, or code gives one to `addFrame()`.

## Animations

A `SpriteAnimation` is a list of frames, each with its duration, a direction (`kForward`, `kReverse`, `kPingPong`: there
and back without showing an end twice) and a repeat count (0: forever). `sample(time)` gives the frame shown at a time
and whether the repetitions are over.

An entity plays one with a `SpriteAnimator`:

```cpp
commands.spawn()
    .add(Transform{.position = {100.0F, 300.0F}})
    .add(SpriteRenderer{.layer = 5})
    .add(SpriteAnimator{.atlas = ship, .animation = animationId("idle")});
```

- `animation` is `animationId(name)`, a hash of the name: the same in every process and independent of the tags' order,
  so a server can replicate which animation plays. To switch, set it and put `time` back to 0.
- `time` is the seconds played at speed 1, `speed` multiplies the frame time (0 freezes), and `playing` stops the clock.
- Every field is reflected. The `rtype.Sprite.Animate` system (PostUpdate, every side) advances the playing animators,
  stops a finite animation on its last frame (`playing` turns false, an `AnimationFinished` event is sent), and writes
  the frame's texture, source and origin into the entity's `SpriteRenderer`, only when they change, so an idle sprite
  stays unchanged for change detection and replication.

## Performance

`examples/plugins/SpriteBenchmark` animates N sprites (10 000 by default) of one atlas, drifting and bouncing, and
prints the frame times. Release build on an Apple M-series laptop (MoltenVK, so presentation waits for the 120 Hz
display): 50 000 animated sprites stay at 120 FPS, 100 000 run at 72 FPS (13.9 ms) and 200 000 at 38 FPS (26.4 ms).
Every sprite of one texture is one instanced draw (see the sprite batching of `docs/Vulkan.md`).
