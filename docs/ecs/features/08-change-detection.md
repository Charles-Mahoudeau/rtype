# 08 · Change Detection

## What it is

Change detection answers **"which components were added or modified since a given moment?"** Each
component instance remembers *when* it was added and *when* it was last written, as a **tick** (a
counter that only goes up). A system compares those ticks with the moment it last ran.

## Why we need it

- **R3 (network):** replication sends only the components that changed since a client's last
  acknowledged state. Without change detection, the server would resend everything every tick.
- **Rendering and other reactive work:** only rebuild what changed (a sprite whose texture changed, a
  UI text whose score changed).
- **Gameplay:** `Added<Player>` to run setup once when a player joins.

## How others do it

| ECS | Mechanism | Granularity |
|---|---|---|
| Bevy | `added` and `changed` ticks per component instance; `Mut<T>` marks on mutable deref | Per instance |
| flecs | Dirty state per table column (`ecs_query_changed`) | Per table |
| Unity DOTS | Version number per chunk, bumped on write access | Per chunk |
| EnTT | Signals (`on_update`) when `patch` / `replace` is used | Per call, opt-in |
| Unreal | Property dirty flags; Iris tracks dirty properties for replication | Per property |

## Our choice

**Per-instance ticks, as Bevy does.** It is the most precise (per entity, per component), and it maps
directly to "send this component of this entity".

## How it works

### Ticks

- The world has a **change tick** counter. It advances every time a system runs.
- Each [pool](04-pool.md) stores, for each instance, `added` and `changed` ticks.
- Each system remembers `lastRun`, the world tick when it last ran.

| World tick | 100 | 101 | 102 |
|---|---|---|---|
| System running | `movement` | `collision`, writes `Position` of `e5` | `replication` (its last run was tick 97) |
| `Position` of `e5` | added 40, changed 40 | added 40, **changed 101** | — |

At tick 102, `replication` sees `Changed<Position>` for `e5` because 101 > 97 (its last run).

| Filter | True when |
|---|---|
| `Added<T>` | `added > lastRun` |
| `Changed<T>` | `changed > lastRun` (adding counts as changing) |

### `Mut<T>`: detecting a write

```c++
template <typename T>
class Mut {
  public:
    [[nodiscard]] const T& get() const noexcept;  ///< Read, does not mark changed.
    [[nodiscard]] T* operator->() noexcept;       ///< Write, marks changed.
    [[nodiscard]] T& operator*() noexcept;        ///< Write, marks changed.

  private:
    T* _value;
    Tick* _changed;
    Tick _currentTick;
};
```

A writable query term yields `Mut<T>`. Reading through `get()` is free; using `->` or `*` stamps
`changed = currentTick`. So a system that iterates `Query<Health>` but only modifies the damaged
entities marks only those.

The type-erased `world.getMut(e, id)` (used by Luau and the network) returns the value's bytes
(`std::span<std::byte>`) and marks on access, since it cannot know what the caller will do with them.
`world.getTicks(e, id)` / `getTicks<T>(e)` return a `ComponentTicks` (`added`, `changed`) with
`isAdded(lastRun, thisRun)` and `isChanged(lastRun, thisRun)`.

### Wraparound

Ticks are 32-bit. At thousands of system runs per second they wrap after weeks, so ticks are compared
by **age** (`isNewerThan(tick, lastRun, thisRun)` compares `thisRun - tick` with `thisRun - lastRun` in
wrapping unsigned arithmetic), and old ticks are **clamped** (Bevy's approach):

- `world.advanceChangeTick(count = 1)` moves the tick forward (the scheduler calls it once per system
  run) and, every `kCheckTickThreshold` (518 400 000) ticks, runs `world.checkChangeTicks()`.
- `checkChangeTicks()` moves every stored `added` / `changed` tick older than `kMaxChangeAge`
  (2^32 − 1 − 2 × the threshold, about 3.26 billion) to exactly that age: it still compares as old, and
  its age can never wrap before the next check.
- Ticks kept **outside** the world (a system's `lastRun`, a client's acknowledged tick) are clamped by
  their owner with `clampTick(tick, thisRun)`.

Comparisons stay correct as long as no system goes more than `kMaxChangeAge` ticks without running.

### Replication's own "last run"

Replication does not use its system's `lastRun`; it keeps **one tick per client**: the tick of the last
snapshot that client acknowledged. "What to send to client A" = everything with `changed` newer than
A's acknowledged tick. A lost packet just makes the next delta larger. See
[Replication](20-replication.md).

## Connections

- Uses: [Pool](04-pool.md) (tick columns), [World](06-world.md) (tick counter).
- Used by: [Queries](07-queries.md) (`Added`, `Changed`, `Mut<T>`), [Systems](12-systems.md)
  (`lastRun`), [Replication](20-replication.md).

## Pitfalls

- **Writing through `operator->` to read.** `position->x` on a `Mut<Position>` counts as a write, even
  if the code only reads. Read with `get()` when nothing changes, or replication sends useless data.
- **Writes that bypass `Mut<T>`.** Any raw pointer write (in C++ or the bridge) must stamp the tick, or
  the change is never replicated.
- **Per-field changes.** A tick says the *component* changed, not which field. Sending only changed
  fields needs a field mask (see the open question).

## Open questions

- Track changes per field (a bitmask per instance) to send only changed fields, or always resend the
  whole component? (ECS + network)
- 32-bit ticks with clamping, or 64-bit and never clamp (8 more bytes per instance)? (ECS)
