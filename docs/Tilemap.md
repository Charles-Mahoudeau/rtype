# Tilemaps and parallax

The tilemap plugin (`src/rtype/plugins/tilemap/`, `rtype::plugins::tilemap`) draws levels made in
[Tiled](https://www.mapeditor.org/) and parallax backgrounds. It knows nothing of the renderer: it pushes the sprites
of the tiles the camera shows to the backend's `SpriteQueue`, which the `BackendPlugin` draws with the sprite entities.
The `BackendPlugin` adds it.

## Loading a map

```cpp
void spawnLevel(ecs::ResMut<TilemapAssets> maps, ecs::Commands& commands) {
    const AssetId level = maps->load("assets/levels/space.tmj");
    spawnTilemap(commands, *maps, level, TilemapPlacement{.position = {0.0F, 0.0F}, .scale = {2.0F, 2.0F}});
}
```

- **`TilemapAssets`** (the resource `rtype.TilemapAssets`) keeps the maps by `AssetId`, as `TextureAssets` keeps the
  textures. `load()` reads a map at once, on every side; one that cannot be read is recorded as failed, with the reason,
  which is printed: a missing map does not stop the game. Its images are handed to `TextureAssets` by the client.
- **`spawnTilemap()`** spawns one entity per layer, bottom first: a `Transform` (the map's position plus the layer's
  offset), a `TilemapLayer` (the map, the layer's index and its draw layer, `firstDrawLayer` plus its index) and a
  `Parallax`. Draw layers are the `SpriteRenderer::layer` of sprites: a map at -100 is under every sprite at 0.

## What is read from Tiled

Save the map as JSON (`.tmj`). The plugin reads an **orthogonal** map, finite or infinite:

| Tiled | Read as |
|---|---|
| Tilesets, embedded or external (`.tsj`) | One image cut into tiles: tile size, columns, spacing, margin, tile offset. A tileset of separate images is rejected. |
| Tile layers | Their tiles, in Tiled's **CSV** layer format (base64 is rejected), with the flip flags (horizontal, vertical, diagonal). A tile bigger than the grid sits on the bottom-left corner of its cell, as in Tiled. |
| Image layers | Their image, repeated along X and/or Y (`repeatx`, `repeaty`) to fill the view. |
| Group layers | Flattened into their layers: offsets added, parallax factors, opacities and tints multiplied, names prefixed (`group/layer`). |
| Every layer | `offsetx`/`offsety`, `parallaxx`/`parallaxy`, `opacity`, `tintcolor`, `visible`. |
| The map | `tilewidth`/`tileheight`, `parallaxoriginx`/`parallaxoriginy`. |
| Custom float properties `scrollx`, `scrolly` | An extension of this plugin: pixels per second the layer scrolls on its own. |

Object layers are skipped. Image and tileset paths are relative to the file that names them.

## Drawing

Tile layers are stored in **chunks** of 16x16 cells, only where there are tiles. Each frame, the Render system
`rtype.Tilemap.Draw` skips the chunks outside the `VisibleArea` (what the camera shows, recorded by the
`BackendPlugin`) and pushes one sprite per tile of the others: a level of any length costs the tiles on screen. All the
tiles of a tileset share its texture, so they are drawn in one batch per layer.

An image layer is one sprite. Repeated, it is still one sprite: its source rectangle spans as many copies as the view
needs, and the texture, asked for with `TextureDesc::repeat`, tiles them.

## Parallax

`Parallax` moves an entity slower or faster than the camera, and scrolls it on its own. Each frame, the Render system
`rtype.Tilemap.Parallax` (before the camera) sets the entity's `GlobalTransform` to its `Transform` plus:

- `(camera - origin) * (1 - factor)`: a factor of 0 stays on screen with the camera, 1 moves with the world, between
  the two moves slower (far away), above 1 faster (a foreground). `origin` is where the camera sees the entity at its
  `Transform`; `spawnTilemap()` puts it at the map's parallax origin.
- `scrolled`, which grows by `scroll` units per second.

It works on any entity, a sprite as well as a tilemap layer. It is a visual effect: it runs on the client only, after
gameplay and the PostUpdate systems, so physics and audio see the entity at its `Transform`.

## In the repository

- `assets/levels/space.tmj`: a 200x10 level over the space background of `assets/backgrounds/space/` (CC0, by
  ansimuz): three repeating layers and two planets at their own parallax speed, a group, and a layer of tiles cut from
  the same images (one tileset embedded, `far-planets.tsj` external), some of them flipped.
- `assets/levels/space-background.tmj`: the three repeating layers alone, each scrolling left at its own speed
  (`scrollx`): the background of the engine's duel.
- `examples/plugins/tilemap`: the camera flies over `space.tmj` (`xmake f --tilemap=y && xmake run tilemap`).
