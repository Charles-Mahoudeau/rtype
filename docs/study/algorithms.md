# Algorithms and data structures

What we took from existing engines, what we wrote ourselves, and why. The details of each part are in its design
document, linked from every row.

## The ECS

The engine is an Entity Component System: the subject recommends it, and it gives the decoupling the subject asks for
(a renderer reads a sprite and a position, nothing else). We compared five existing ECSs (Unity DOTS, Unreal Mass,
flecs, Bevy, EnTT) part by part; the full comparison is [`ecs/design.md`](../ecs/design.md).

| Part | Options | Our choice | Why |
|---|---|---|---|
| [Entity](../ecs/features/01-entity.md) | Plain index; index + generation; pointer | **32-bit index + 32-bit generation** | A stale handle is detected (`isAlive()`), ids are reused safely |
| [Storage](../ecs/features/04-pool.md) | Archetype tables (flecs, DOTS); sparse sets (EnTT); both (Bevy) | **Sparse sets with tick columns today; archetype tables designed as a per-component option** ([05](../ecs/features/05-archetype-tables.md)) | R-Type's entities change composition often (bullets, states, script components): sparse sets make adding and removing cheap. Tables would iterate faster for large stable sets, for later games |
| [Sparse array](../ecs/features/03-sparse-array.md) | Flat array; hash map; paged array | **Paged sparse array, pages of 4096 entries, allocated on first use** | O(1) lookup without reserving memory for every possible entity |
| [Queries](../ecs/features/07-queries.md) | Typed only; dynamic only | **Typed queries compiled down to dynamic queries** | One iteration algorithm to test and optimize, and Luau and tools use the same one |
| [Change detection](../ecs/features/08-change-detection.md) | Dirty flags; per-system ticks; per-instance ticks | **Per-instance added/changed ticks** (Bevy) | Exact "what changed since this system last ran", and exactly "send this component of this entity" for replication |
| [Commands](../ecs/features/09-commands.md) | Immediate changes; deferred buffer | **Deferred buffer flushed in recording order at the end of each phase**; `spawn()` reserves an id at once | Iterating a query while spawning stays safe |
| [Events](../ecs/features/11-events.md) | Observers (immediate callbacks); queues | **Double-buffered queues** (Bevy) | All logic stays in scheduled systems, in a predictable order |
| [Scheduler](../ecs/features/13-scheduler.md) | Free order; phases; dependency graph | **Phases + explicit ordering + fixed timestep**; access sets recorded | Deterministic order now, a parallel scheduler possible later |
| [Hierarchy](../ecs/features/17-hierarchy.md) | Relationship pairs (flecs); parent/children components | **`Parent` / `Children` kept by the `World`, plus a propagation system** | Covers UI, scenes and attached objects without a relationship engine |
| [Reflection](../ecs/features/15-reflection-and-serialization.md) | Compile-time only; runtime type info | **A runtime `TypeInfo` per type**: fields, offsets, types | One description serves scripts, scene files, the editor and the network |

**Time.** Gameplay runs on a fixed timestep (60 ticks per second by default), decoupled from the frame rate, as the
subject requires: the starfield and every entity move by elapsed time, never by frame.

## Replication

Full design: [`network/design.md`](../network/design.md); wire format: [`network/protocol.md`](../network/protocol.md).

| Problem | Options | Our choice | Why |
|---|---|---|---|
| What to send | Full state every tick; deltas from the previous tick; deltas from the last **acknowledged** tick | **States changed since the last snapshot the client acknowledged** (Quake 3, bevy_replicon), per entity and per component | A lost snapshot is repaired by the next one with no resend logic; nothing is sent twice once confirmed |
| Bullets | Replicate every tick; spawn only | **`kSpawnOnly` components**: sent at spawn, then simulated on both sides | Hundreds of bullets cost ~20 bytes each once, instead of every tick |
| Smooth display | Show the latest snapshot; extrapolate; interpolate | **Interpolate ~100 ms in the past** between two snapshots (designed, not implemented yet) | No stutter when snapshots arrive unevenly |
| The local ship | Wait for the server; predict | **Client-side prediction with server reconciliation**: each snapshot carries the last input the server applied (`lastProcessedInput`), the client replays the later ones (physics `moveAndSlide`) | Movement answers at once; the server still decides |
| Inputs | Events ("pressed"); states ("held") | **Held states, the last 3 samples in every message** | A lost datagram is covered by the next one, with no resend delay |
| Agreeing on the game | Trust; a version number; a manifest of layouts | **A manifest** of components and actions, with layout hashes, sorted by name | Short ids on the wire; a client with other scripts is refused instead of misreading bytes |

## Reliability over UDP

| Mechanism | Algorithm | Notes |
|---|---|---|
| Sequence numbers | 16-bit, compared with wrap-around (`isNewer`) | RFC 1982 serial number arithmetic |
| Acknowledgements | `ack` + 32 `ackBits` in every datagram | Each datagram acknowledges 33; lost acknowledgements are covered by the next ones (Gaffer On Games) |
| Reliable channel | Message ids, resend after ~1 RTT, a receive window of 256, in-order delivery | A ring buffer (`SequenceBuffer`) indexed by sequence modulo its size, O(1) |
| Keepalive and timeout | Empty datagram after 100 ms idle, lost after 2 s | Values are settings |
| Anti-spoofing | Challenge, random session token, padded requests | From the netcode standard |

## Custom algorithms, and why we wrote them

| What | Why not a library |
|---|---|
| The ECS itself | The subject asks for an engine of our own; and only our own ECS could carry per-component replication modes, Luau-declared components and capsules natively |
| The reliability layer | ENet and the like do not know about snapshots and acknowledged ticks; ours is about 700 lines and tested with a simulated lossy link (`network/simulation/LinkSimulator`) |
| The bounds-checked byte reader/writer | Every read is checked and a failed read invalidates the reader, so a decoder reads a whole message and checks once: no exceptions on network data |
| Input quantization | Axes in [-1, 1] as `i16` (×32767): half the bytes of a float, with a precision far below what a stick reports |
| Name hashes | FNV-1a 64-bit of a component's name identifies it across shared libraries (plugins), where a per-type counter would differ per library |
| The scene file format | Keeps the file's order so an editor's save changes only what was edited; compared with TOML in [`../benchmarks/capsule-format.md`](../benchmarks/capsule-format.md) |
| Signed-distance-field text | One small atlas per font stays sharp from 12 to 200 pixels ([`Text.md`](../Text.md)) |
| GPU particles | Simulated in compute shaders, with a CPU fallback ([`particles/design.md`](../particles/design.md)) |

## Design patterns

| Pattern | Where |
|---|---|
| Entity Component System | The whole engine |
| Plugin (with a dependency graph, no cycles) | Engine features (`plugin-*`), loaded from shared libraries |
| Abstract factory + registry | Backends behind contracts (`IRenderer`, `IPlatform`, audio): chosen by name in `project.luau` |
| Mediator / event bus | ECS events between systems that do not know each other |
| Command | `Commands` buffers deferred world changes |
| Result type | Failures a correct program can hit are returned (`Result`, `expected`), misuse throws |
| Producer / consumer queues | The server's network thread and room threads exchange messages through two queues |
