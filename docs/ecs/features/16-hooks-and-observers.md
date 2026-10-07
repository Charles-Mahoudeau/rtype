# 16 · Hooks and Observers

## What it is

**Hooks** are functions the storage calls when a component's bytes are created, moved, copied or
destroyed. **Observers** are callbacks that run immediately when something happens to a component
(added, removed, set) or when an event fires.

Our ECS has hooks from v1, but **only for memory**. Observers are left for later.

## Why we need it

- **Hooks:** the [pool](04-pool.md) stores raw bytes. For a C++ component with a `std::vector` member,
  copying bytes with `memcpy` would be wrong; the storage must call the C++ constructor, move
  constructor and destructor. Hooks are how a type-erased storage does that.
- **Observers:** convenient for reactions ("when `Sprite` is added, load its texture"), but they run
  logic outside the schedule.

## How others do it

| ECS | Lifecycle hooks | Observers |
|---|---|---|
| flecs | `ctor`, `dtor`, `move`, `copy`, plus `on_add` / `on_remove` / `on_set` hooks | Observers on any event, with queries |
| Bevy | Component hooks (`on_add`, `on_insert`, `on_replace`, `on_remove`) | Observers and triggers |
| EnTT | Constructors run by typed storage | Signals: `on_construct`, `on_update`, `on_destroy` |
| Unity DOTS | Structs only (no lifecycle) | None built in |

## Our choice

| Mechanism | v1 | Purpose |
|---|---|---|
| **Memory hooks** (`construct`, `destruct`, `move`, `copy`) | Yes | Manage the component's own memory, nothing else |
| Gameplay reactions | Through `Added<T>` filters and [events](11-events.md) | Keep all logic in scheduled systems |
| Observers | v3, if a real need appears | Immediate reactions that cannot wait for a phase |

## How it works

### Memory hooks

```c++
struct ComponentHooks {
    void (*construct)(void* dst){nullptr};
    void (*destruct)(void* dst){nullptr};
    void (*move)(void* dst, void* src){nullptr};
    void (*copy)(void* dst, const void* src){nullptr};
};
```

`RTYPE_ECS_COMPONENT` generates them from the C++ type, and leaves them **null** when the type is
trivially copyable. A null hook means "use `memset` / `memcpy` / nothing", which is the fast path and
the case for every Luau component.

| Storage action | Hook called | When null |
|---|---|---|
| `emplace` | `construct` | zero-fill |
| swap-and-pop `remove` | `move` (last → hole), then `destruct` (last) | `memcpy`, nothing |
| Column growth | `move` for each element | `memcpy` of the whole block |
| [Prefab](18-prefabs-and-scenes.md) instantiation | `copy` | `memcpy` |

### Reactions without observers

"When a player joins, give it a ship" and "when `SpriteRenderer` is added, make sure its texture is
loaded" are written as systems:

```c++
void loadNewSprites(rtype::ecs::Query<const SpriteRenderer, rtype::ecs::Added<SpriteRenderer>> sprites,
                    rtype::ecs::ResMut<rtype::engine::assets::Assets<rtype::engine::graphics::Texture>> textures) {
    for (auto [entity, sprite] : sprites) {
        textures->request(sprite.texture);
    }
}
```

The reaction happens at a known point of the frame, in a known order with other systems, and is visible
in the schedule. An observer would run in the middle of whatever triggered it.

### When observers would be worth it

- Keeping an **index** in sync (a spatial grid that must update the moment a `Collider` is removed).
- **Editor tooling** that reacts to every change.

If those needs appear, observers can be added on top of the same events, without touching hooks.

## Connections

- Uses: [Component registry](02-component-registry.md) (hooks live in `ComponentInfo`).
- Used by: [Pool](04-pool.md), [Archetype tables](05-archetype-tables.md),
  [Prefabs](18-prefabs-and-scenes.md).

## Pitfalls

- **Gameplay in hooks.** A `construct` hook that plays a sound would run during a flush, in storage
  code, with no access to the world. Hooks only touch the component's own bytes.
- **Throwing from a hook.** A hook runs in the middle of a storage operation; it must not throw (`move`
  and `destruct` are `noexcept`).
- **Non-trivial Luau components.** There are none: Luau components are plain data by design.

## Open questions

- Is there an R-Type feature that truly needs immediate observers (a spatial index for collisions)?
  (Team, after `CollisionPlugin` design)
