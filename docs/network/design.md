# Network Design

This document specifies the network of the R-Type engine: the transport library (`rtype-network`), the
replication library that connects it to the ECS (`rtype-replication`), and the `client` and `server`
executables. Nothing here is implemented yet. For a shorter first read, see [`overview.md`](overview.md).

It builds on the ECS design, which already decides *what* is replicated and *why*:
[`../ecs/design.md`](../ecs/design.md#networking) and
[`../ecs/features/20-replication.md`](../ecs/features/20-replication.md) (inputs up, state down, replication
modes, manifest), [`08-change-detection.md`](../ecs/features/08-change-detection.md) (change ticks) and
[`15-reflection-and-serialization.md`](../ecs/features/15-reflection-and-serialization.md) (`FieldInfo`). This
document decides *how*: the wire format, reliability, connections, threads and robustness. Where it changes
something the ECS docs say, the change is listed in [Differences with the ECS docs](#13-differences-with-the-ecs-docs).

> Snippets omit the Epitech header, the rule-of-five members and most `[[nodiscard]]`/`noexcept` for brevity.
> The real code follows every rule in `CLAUDE.md`.

## Contents

1. [Requirements](#1-requirements)
2. [Principles](#2-principles)
3. [Targets and dependencies](#3-targets-and-dependencies)
4. [Topology and run modes](#4-topology-and-run-modes)
5. [Threads](#5-threads)
6. [Transport: `rtype-network`](#6-transport-rtype-network)
7. [Replication: `rtype-replication`](#7-replication-rtype-replication)
8. [Custom packets](#8-custom-packets)
9. [Lobby and rooms](#9-lobby-and-rooms)
10. [Robustness and security](#10-robustness-and-security)
11. [Bandwidth estimate](#11-bandwidth-estimate)
12. [Testing](#12-testing)
13. [Differences with the ECS docs](#13-differences-with-the-ecs-docs)
14. [Pitfalls](#14-pitfalls)
15. [Build order](#15-build-order)
16. [Open questions](#16-open-questions)
17. [References](#17-references)

## 1. Requirements

| # | Requirement | Source | Met by |
|---|---|---|---|
| N1 | The server notifies every client when a monster spawns, moves, is destroyed, fires, kills a player, and of the other clients' actions | Subject | [Snapshots](#74-snapshots-server--client) (spawns, updates, despawns; a shot is a bullet spawn) and [custom packets](#8-custom-packets) (`PlayerKilled`...) |
| N2 | The server is multithreaded: it never blocks or waits for client messages, the game runs frame after frame regardless of clients | Subject | One I/O thread plus one thread per room; rooms never wait on the network ([Threads](#5-threads)) |
| N3 | If a client crashes, the server keeps working and notifies the other clients of the same game | Subject | Timeouts, `kClientLeft` broadcast, `ClientDisconnected` event ([Connection lifecycle](#63-connection-lifecycle)) |
| N4 | The server is robust: it keeps running whatever goes wrong | Subject | [Robustness and security](#10-robustness-and-security) |
| N5 | Networking uses Asio or the OS socket API | Subject | Standalone Asio; the reliability layer is written in-house (ENet and similar libraries are not allowed) |
| N6 | Authoritative server, inputs up, state down, no client prediction | ECS R3 | The whole design |
| N7 | Builds and runs on Linux, macOS and Windows with Clang, Apple Clang and MSVC | `CLAUDE.md` | No compiler extensions; [Windows pitfall](#windows-connection_reset) handled |

## 2. Principles

- **The server decides, clients display.** Clients send inputs and never state. The server never trusts what
  a client sends beyond "these buttons are held".
- **The transport knows nothing about the ECS.** `rtype-network` moves bytes and messages between endpoints.
  Only `rtype-replication` knows both sides.
- **One owner per piece of data.** Every variable is written by exactly one thread. Threads talk through
  queues, never through shared variables.
- **Send states, not differences.** A snapshot says "health is 40", never "health lost 10". Receiving a state
  twice is harmless, so lost packets are repaired by sending the current state again, never by resending
  old packets.
- **Never trust the network.** Every byte read from a socket is checked. A bad packet is dropped; it never
  crashes, throws or asserts.
- **Names are the identity, short ids are the wire.** Components, packets, prefabs and input actions are
  identified by name. The server sends a manifest (name, hash, short id) at connection; the wire then uses
  the 2-byte short ids.

## 3. Targets and dependencies

```mermaid
flowchart BT
    core["engine-core<br/>(lib)"]
    ecs["ecs<br/>(lib)"]
    luau["luau<br/>(lib)"]
    net["network<br/>(lib, Asio only)"]
    repl["replication<br/>(lib)"]
    engine["engine<br/>(lib)"]
    glfw["platform-glfw"]
    vk["render-vulkan"]
    client["client<br/>(executable)"]
    server["server<br/>(executable, headless)"]

    core --> engine
    ecs --> engine
    luau --> engine
    ecs --> repl
    net --> repl
    engine --> client
    repl --> client
    glfw --> client
    vk --> client
    engine --> server
    repl --> server
```

An arrow means "is used by".

| Target | Kind | Folder / namespace | Contains | Depends on |
|---|---|---|---|---|
| `network` | library (`rtype-network`) | `src/rtype/network/`, `rtype::network` | UDP socket and I/O thread, handshake and sessions, sequence numbers and acks, reliable channel, message packing, `ByteWriter` / `ByteReader`, lobby routing | Asio only. No ECS, no engine. |
| `replication` | library (`rtype-replication`) | `src/rtype/replication/`, `rtype::replication` | `ServerPlugin` / `ClientPlugin` and their systems, `NetworkId` / `Owner` / `PlayerInput` / `Interpolated`, manifest, snapshot writer and reader, per-client ack bookkeeping, input buffers, custom packet registry | `ecs`, `network` |
| `engine` | library (today the executable) | `src/rtype/engine/` | The engine shared by both executables | `engine-core`, `ecs`, `luau` |
| `client` | executable | | The game with a window. Can also host a game ([listen server](#4-topology-and-run-modes)). | `engine`, `replication`, `platform-glfw`, `render-vulkan` |
| `server` | executable, headless | | The dedicated server | `engine`, `replication`. **Never** `platform-glfw` or `render-vulkan`, so it starts on a machine with no display or GPU. |

Rules, in the spirit of the existing dependency rules:

- `network` and `ecs` never include each other's headers. The glue lives in `replication`.
- `network` is declared with `add_requires("asio 1.36.x")` in its own `xmake.lua`. Exported symbols use
  `RTYPE_NETWORK_API` / `RTYPE_REPLICATION_API` if the libraries are shared.
- No header outside `network/` includes `<asio.hpp>`, just as GLFW never leaves `platform/glfw/`.
- Tests: `tests/network/` (`network-tests`) and `tests/replication/` (`replication-tests`).

## 4. Topology and run modes

**Authoritative client-server over UDP.** Peer-to-peer is rejected because it needs deterministic lockstep
(floating-point and Luau differences desynchronize peers, and one slow peer freezes everyone) or rollback
(rewinding the Luau VMs), and it fails behind strict NATs. TCP is rejected for gameplay because one lost
packet blocks everything behind it until it is resent (head-of-line blocking), which shows as freezes.

| Mode | Process | Network |
|---|---|---|
| Dedicated server | `server` | Binds one UDP port (default `4242`) and hosts every room. |
| Client | `client` | Connects to a server. |
| Listen server ("Host" button) | `client` | Starts a **server thread inside the client process**: the same server code, with its own rooms, worlds and Luau VMs, bound on `0.0.0.0`. The client connects to `127.0.0.1` like any other player; friends connect to the host's IP. Closing the game stops and joins that thread. |
| Standalone (single player) | `client` | No network at all: the ECS `StandalonePlugin` runs server and client systems in one world. |

The listen server runs in a thread rather than a child process: there is no executable to locate, no
per-OS process API (`CreateProcess`, `posix_spawn`), and no orphaned server left running if the client
crashes. It works because the ECS keeps no global state and each `luau::Runtime` owns its own `lua_State`.

## 5. Threads

### Server

```mermaid
flowchart LR
    subgraph io["I/O thread (owns the socket and every session)"]
        recv["receive datagram<br/>check token, update acks,<br/>split into messages"]
        lobby["lobby: room list,<br/>join, leave"]
        send["pack messages per client,<br/>add header, send"]
    end
    subgraph room1["Room 1 thread (App: World + Luau VM)"]
        r1in["PreUpdate: net.receive"]
        r1sim["FixedUpdate: gameplay"]
        r1out["PostUpdate: net.snapshot, net.send"]
    end
    subgraph main["Main thread"]
        rooms["RoomManager:<br/>create and stop rooms"]
    end
    recv -- "inbox (per room)" --> r1in
    r1out -- "outbox (per room)" --> send
    lobby -- "create room requests" --> rooms
    rooms -- "register room inbox/outbox" --> lobby
```

| Thread | Owns | Never touches |
|---|---|---|
| I/O thread (one) | The socket, every `Session` (endpoint, token, sequences, acks, reliable queues, RTT, room id), the room table | Any `World`, any `lua_State` |
| Room thread (one per room) | That room's `App`: `World`, schedule, Luau `Runtime`, replication state | The socket, sessions, other rooms |
| Main thread | The `RoomManager`: creates rooms and their threads, joins them, handles shutdown signals | Sessions, worlds |

The `RoomManager` lives in `replication`, so the listen server reuses it. The executable gives it a factory
that builds a room's `App` with the game's plugins.

- **Rooms only see messages.** A room never sees sequence numbers, acks or datagrams. It receives
  `ClientJoined`, `ClientLeft` and `Message{client, channel, bytes}` from its inbox, and pushes
  `Send{client or everyone, channel, bytes}` and `Kick{client, reason}` into its outbox.
- **The lobby runs on the I/O thread**, because it changes `Session::roomId`, which the I/O thread reads to
  route every packet.
- **Queues are a `std::mutex` and a `std::vector`.** Once per tick the room locks, swaps the vector with an
  empty one and unlocks; the lock is held for nanoseconds. After pushing to its outbox, a room posts a flush
  to the I/O thread's `asio::io_context`, so its packets leave right away. Lock-free queues are not worth
  their complexity at 60 Hz with a few clients.
- **One thread per room** means a slow Luau script in one room cannot slow down another. The number of rooms
  is capped (`kMaxRooms`).
- **Each room is its own `App`** with its own schedule, running a fixed 60 Hz loop.

### Client

The client has **no network thread**. At the start of every frame (`PreUpdate`), `net.receive` calls
`io_context::poll()`, which handles the datagrams that arrived and the due resends, then returns at once.
At 30 frames per second or more, that is often enough for acks and resends. A listen server's server part
still has its own I/O and room threads.

### Two different ticks

- The **server tick** counts fixed updates (60 per second). It numbers snapshots.
- The ECS **`Tick`** ([`change/Tick.hpp`](../../src/rtype/ecs/change/Tick.hpp)) counts system runs. It stamps
  component changes.

When the server sends the snapshot of server tick 101, it records the ECS tick of that moment (say 5432).
"Changed since this snapshot" then means "change tick greater than 5432".

## 6. Transport: `rtype-network`

### 6.1 Bytes on the wire

- **Little-endian everywhere**, written byte by byte with shifts, so the result does not depend on the
  machine.
- **No serialization library.** Replication writes type-erased component bytes field by field from
  `FieldInfo`, which a compile-time serializer cannot do, and it needs quantization later. A small writer and
  reader cover every need:

```c++
namespace rtype::network {
/// @brief Writes little-endian values into a fixed buffer. Once a write overflows, it is invalid for good.
class ByteWriter {
  public:
    explicit ByteWriter(std::span<std::byte> buffer) noexcept;

    void writeU8(std::uint8_t value) noexcept;
    void writeU16(std::uint16_t value) noexcept;
    void writeU32(std::uint32_t value) noexcept;
    void writeU64(std::uint64_t value) noexcept;
    void writeF32(float value) noexcept;  // std::bit_cast<std::uint32_t>(value)
    void writeBytes(std::span<const std::byte> bytes) noexcept;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] std::size_t getSize() const noexcept;
};

/// @brief Reads little-endian values. Reading past the end returns 0 and makes the reader invalid for good.
class ByteReader {
  public:
    explicit ByteReader(std::span<const std::byte> data) noexcept;

    [[nodiscard]] std::uint8_t readU8() noexcept;
    [[nodiscard]] std::uint16_t readU16() noexcept;
    // ... same types as ByteWriter

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] std::size_t getRemaining() const noexcept;
};
}  // namespace rtype::network
```

A parser reads every field, then checks `isValid()` once. A truncated packet can never read out of bounds.

### 6.2 Packets

Every datagram starts with a **packet kind** byte.

| Value | Kind | Direction | Content after the kind byte |
|---|---|---|---|
| `0x01` | `kConnectRequest` | C → S | protocol id `u32` (`0x52545950`, "RTYP"), protocol version `u16`, zero padding up to 64 bytes in total |
| `0x02` | `kChallenge` | S → C | challenge `u64` |
| `0x03` | `kChallengeResponse` | C → S | challenge `u64`, zero padding up to 64 bytes in total |
| `0x04` | `kAccepted` | S → C | session token `u32`, client id `u32` |
| `0x05` | `kRejected` | S → C | reason `u8` (`kVersionMismatch`, `kServerFull`) |
| `0x06` | `kData` | both | session token `u32`, sequence `u16`, ack `u16`, ack bits `u32`, then messages ([6.5](#65-messages-and-channels)) |
| `0x07` | `kDisconnect` | both | session token `u32`, reason `u8` |

The `kData` header is 13 bytes. Limits:

| Constant | Value | Why |
|---|---|---|
| `kMaxPacketSize` | 1200 bytes | Below every common MTU, IPv6's minimum of 1280 included, minus IP and UDP headers, so the OS never fragments a datagram (one lost fragment loses the whole datagram). |
| `kMaxMessageSize` | 1000 bytes | Leaves room for the header and a few reliable messages in the same datagram. |

A datagram larger than `kMaxPacketSize`, or of a size that does not match its kind, is dropped.

### 6.3 Connection lifecycle

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Server (I/O thread)
    C->>S: kConnectRequest (protocol id, version, padding)
    Note over S: no session yet, only a pending challenge
    S->>C: kChallenge (random u64)
    C->>S: kChallengeResponse (same u64, padding)
    Note over S: matches the pending challenge of this ip:port → create Session
    S->>C: kAccepted (session token, client id)
    S->>C: welcome messages: the manifest (reliable)
    C->>C: check the manifest, refuse on mismatch
    C->>S: kReady (reliable)
    Note over S: the client is in the lobby
    loop until disconnect
        C-->>S: kData
        S-->>C: kData
    end
    C->>S: kDisconnect (graceful), or nothing for 2 s (timeout)
    Note over S: kClientLeft to the room, ClientLeft to the room's inbox
```

- **Challenge.** The server answers a connect request with a random `u64` and remembers it in a bounded
  table (`kMaxPendingChallenges` = 256 entries, 5 s expiry). A session exists only once the client echoes the
  challenge from the same `ip:port`. Someone forging another player's address never receives the challenge,
  so they cannot create sessions or use memory.
- **Padding.** Both client handshake packets are padded to 64 bytes and every server reply is smaller, so the
  server can never be used to send more bytes to a forged address than it received (amplification).
- **Session token.** A random `u32` given in `kAccepted`, carried by every `kData` and `kDisconnect`. A
  packet whose token does not match its endpoint's session is dropped, so forged inputs are rejected. Tokens
  and challenges come from a `std::mt19937_64` seeded by `std::random_device`; that is not cryptographic, but
  enough for this scope (see [Security scope](#security-scope)).
- **Version.** A `kConnectRequest` with another protocol version gets `kRejected{kVersionMismatch}`, so the
  client can show a clear message.
- **Lost handshake packets.** The client resends `kConnectRequest`, then `kChallengeResponse`, every 100 ms
  and gives up after 5 s ("server unreachable"). If `kAccepted` is lost, the client resends its response; the
  server sees a session created with that same challenge and resends `kAccepted`.
- **Keepalive.** If a side has sent no `kData` for 100 ms, it sends an empty one, which also carries its acks.
- **Timeout.** A session that receives nothing for 2 s is closed with reason `kTimeout`. This is how a
  crashed client is detected (N3).
- **Disconnect.** A graceful `kDisconnect` is sent 3 times, since nothing acks it. Reasons: `kGraceful`,
  `kTimeout`, `kKicked`, `kProtocolError`, `kManifestMismatch`, `kOverflow`, `kServerShutdown`.

When a session closes for any reason, the I/O thread sends a reliable `kClientLeft{clientId, reason}` to the
other clients of its room and pushes `ClientLeft` to the room's inbox. The server keeps running either way.

### 6.4 Sequence numbers, acks and RTT

Every `kData` has its own `sequence`, incremented by one per datagram sent to that peer.

- `ack` is the newest sequence received from the peer.
- Bit `N` of `ackBits` is set if sequence `ack - 1 - N` was received too.

Every datagram therefore confirms the last 33 datagrams received, and a lost ack is repaired by the next
datagram. Acks are always piggybacked; no packet exists only to ack.

**Wraparound.** A `u16` counts to 65535, then goes back to 0: after about 18 minutes at 60 datagrams per
second. Comparing with `<` would then think every new packet is old and drop them all. Every comparison of
sequences (and of reliable message ids) goes through:

```c++
/// @return True if sequence `a` is newer than `b`, across the wrap from 65535 to 0.
[[nodiscard]] constexpr bool isNewer(std::uint16_t a, std::uint16_t b) noexcept {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(a - b)) > 0;
}
// isNewer(3, 65530) == true: 3 is 9 steps ahead of 65530.
```

**Sent packet records.** The sender keeps a ring of the last 1024 datagrams (index `sequence % 1024`): send
time, acked flag, and the reliable message ids it carried. When a datagram is acked, its reliable messages
are removed from the resend queue ([6.5](#65-messages-and-channels)).

**RTT** is measured from acks: when a datagram is acked for the first time, `sample = now - sendTime` and
`rtt += (sample - rtt) * 0.1`. No ping packets are needed.

### 6.5 Messages and channels

A `kData` datagram carries a list of messages, as many as fit in `kMaxPacketSize`:

```
kData datagram (≤ 1200 bytes)
├─ header (13 bytes): kind | session token | sequence | ack | ack bits
└─ messages, back to back until the end of the datagram
   ├─ unreliable: channel u8 = 0 | length u16 | payload
   └─ reliable:   channel u8 = 1 | message id u16 | length u16 | payload
```

The transport never looks inside a payload. The first payload byte is the **message type**, defined by the
layer that owns it: `0x00`–`0x1F` by `rtype-network` (lobby, [9](#9-lobby-and-rooms)), `0x20`–`0xFF` by
`rtype-replication` ([7](#7-replication-rtype-replication)).

**Two channels:**

| Channel | Guarantee | Used for |
|---|---|---|
| `kUnreliable` (`0`) | None: can be lost, duplicated or reordered | Snapshots and inputs. They carry ticks, so the layer above drops old ones itself. |
| `kReliable` (`1`) | Delivered exactly once, in order | Manifest, `kReady`, lobby messages, `kClientLeft`, reliable custom packets (`PlayerKilled`) |

There is no reliable unordered channel. Its only benefit is avoiding head-of-line blocking, which does not
matter once spawns and despawns travel in snapshots and reliable traffic is rare.

**Why reliability is per message, not per datagram.** A resent datagram needs a new sequence number, or acks
become ambiguous, so it is the *content* that is resent and that content needs its own id. That id also lets
the receiver drop duplicates: if a datagram arrives but its ack is lost, the sender resends the message, and
without an id it would be delivered twice (two kill-feed lines, or two actions).

**Reliable sender:**

1. A new reliable message gets the next message id and joins the resend queue.
2. When a datagram is built, it takes the queued messages not sent within the last
   `max(1.25 × RTT, 50 ms)` (oldest first), and the datagram's record notes their ids.
3. When that datagram is acked, those messages leave the queue.
4. At most `kReliableWindow` (256) messages are in flight. The ids are `u16`, so a bounded window keeps
   `isNewer` unambiguous. If more than `kMaxPendingReliable` (1024) messages pile up, the peer is not keeping
   up and is disconnected with `kOverflow`.

**Reliable receiver:** it keeps the next expected id.

- A message with the expected id is delivered, followed by any consecutive ones kept aside.
- A message ahead of it (within the window) is kept aside.
- A message older than it is a duplicate and is dropped.

**Datagram packing order:** reliable messages due for resend, then new reliable messages, then unreliable
ones. An unreliable message that does not fit is dropped: snapshots are sized to fit
([7.4](#74-snapshots-server--client)).

### 6.6 Large data

There is **no fragmentation** in v1. A message is never larger than `kMaxMessageSize`; anything bigger is
split by its sender into several reliable messages. The reliable channel guarantees they all arrive, in
order. For example, the manifest is sent as one message per entry followed by `kManifestEnd`
([7.2](#72-manifest)).

If larger blobs are needed later (a level file), a generic split is easy on top of the ordered channel: each
piece carries a "more follows" flag and the receiver concatenates them.

### 6.7 Socket and I/O

- One `asio::ip::udp::socket` bound to one port serves every client and every room. Datagrams are matched
  to a `Session` by their source endpoint and session token. One port per room is avoided because of
  firewalls and NATs.
- The server receives with `async_receive_from` and re-arms the receive after **every** result, error or
  not, except `operation_aborted` (shutdown).
- A `steady_timer` on the I/O thread (every 10 ms) runs resends, keepalives, timeouts and challenge expiry.
- On Windows, `_WIN32_WINNT` is defined to silence Asio's warning, and the socket gets `SIO_UDP_CONNRESET`
  turned off (see [Windows `connection_reset`](#windows-connection_reset)).

## 7. Replication: `rtype-replication`

This library implements [`20-replication.md`](../ecs/features/20-replication.md) on top of `rtype-network`.
The ECS side (components, resources, systems, replication modes) is described there; this section fixes
the wire format and the bookkeeping.

### 7.1 Identity

- Every replicated entity gets a `NetworkId` (`u32`) from the server: per room, increasing, **never
  reused**, `0` means none. At 1000 spawns per second it would take 49 days to run out.
- The client keeps `NetworkId → Entity` in `ClientState`. A `kEntity` field is written as the target's
  `NetworkId` (`0` if null or not replicated) and read back through that map; an unknown id becomes the
  null `Entity`.
- `Entity` values are never sent: they are local to one world.
- Client ids (`u32`) are given by the server in `kAccepted`, starting at 1.

### 7.2 Manifest

The manifest is what makes names into short ids and catches every mismatch between the client's and the
server's game.

| Entry | Content | Checked with |
|---|---|---|
| Component | short id `u16`, name, `layoutHash` `u64` | The client's `ComponentInfo::layoutHash` for that name |
| Custom packet | short id `u16`, name, schema hash `u64` | The client's schema hash for that name ([8](#8-custom-packets)) |
| Prefab | short id `u16`, name | The client knows the name |
| Input action | index `u8`, name, kind `u8` (`kButton`, `kAxis`, `kVector2`) | Same name and kind at the same index |

- Short ids are indices in the manifest, entries sorted by name. They are **not** the world's dense
  `ComponentId`s: each world maps short id → its own `ComponentId` by name.
- Names are written as a length `u8` followed by the bytes (at most 255).
- At startup the server builds one `App` with the game's plugins (without running it) to produce the
  manifest. Every room's `App` must produce the same manifest hash, checked when the room starts.
- The network library sends the manifest as **welcome messages** to every new session right after
  `kAccepted`, without knowing what they contain.
- The client compares every entry. On any unknown name or different hash, it disconnects with
  `kManifestMismatch` and shows which entry differs ("`game.PlayerKilled` differs between client and
  server: update your scripts"). Otherwise it sends `kReady`.

### 7.3 Message types

| Value | Type | Channel | Direction | Content |
|---|---|---|---|---|
| `0x20` | `kManifestComponent` | reliable | S → C | short id `u16`, name, layout hash `u64` |
| `0x21` | `kManifestPacket` | reliable | S → C | short id `u16`, name, schema hash `u64` |
| `0x22` | `kManifestPrefab` | reliable | S → C | short id `u16`, name |
| `0x23` | `kManifestInputAction` | reliable | S → C | index `u8`, name, kind `u8` |
| `0x24` | `kManifestEnd` | reliable | S → C | entry count `u16`, manifest hash `u64` |
| `0x30` | `kSnapshot` | unreliable | S → C | [7.4](#74-snapshots-server--client) |
| `0x31` | `kInput` | unreliable | C → S | [7.5](#75-inputs-client--server) |
| `0x32` | `kCustomPacket` | packet's channel | packet's direction | short id `u16`, fields ([8](#8-custom-packets)) |

### 7.4 Snapshots (server → client)

**Layout:**

```
u8   type = kSnapshot
u32  tick                    server tick of this snapshot
u16  spawn count
     spawn   × { netId u32, prefab u16 (0xFFFF = none), component count u8,
                 component count × { component u16, fields } }
u16  update count
     update  × { netId u32, component count u8, component count × { component u16, fields } }
u16  removal count
     removal × { netId u32, component u16 }
u16  despawn count
     despawn × { netId u32 }
```

Fields are written by walking the component's `FieldInfo` list (one generic serializer for C++ and Luau
components), with the quantization options of
[`15-reflection-and-serialization.md`](../ecs/features/15-reflection-and-serialization.md) when set. v1 sends
whole components; per-field masks come later.

**What goes in a snapshot for one client.** The server keeps, per client, the ECS tick at which the client
last confirmed each entity (`knownEntities: NetworkId → Tick`):

| Entity state for that client | Sent as |
|---|---|
| Not in `knownEntities` (never confirmed) | **spawn**, with all its replicated components (`kEveryChange` and `kSpawnOnly`) |
| Known, a `kEveryChange` component changed after its known tick | **update** with those components; a component added since is sent the same way |
| Known, a replicated component removed after its known tick | **removal** |
| Despawned, and it was sent to this client at least once | **despawn**, until confirmed |

So spawns and despawns are **in the snapshot**, not on a reliable channel. A spawn is repeated in every
snapshot until the client confirms it (about one round trip, so 1 to 3 times), then never again. Lost
packets are repaired automatically, and one lost bullet spawn never delays anything else. An entity that
spawns and despawns before any snapshot includes it is never sent at all.

**Confirmations are application-level.** Each `kInput` carries the tick of the last snapshot the client
**applied** ([7.5](#75-inputs-client--server)). For the last 64 snapshots sent to each client, the server
keeps the ECS tick at that moment and the list of entities, removals and despawns the snapshot contained.
When the client reports snapshot `T` as applied, every entity included in `T` gets `knownEntities[e] =
ecsTick(T)`, and the removals and despawns of `T` are cleared. A report too old to be in the ring is ignored,
so the data is simply sent again.

Packet-level acks cannot be used for this: a client may receive a snapshot and drop it because a newer one
already arrived, and the transport would still ack the datagram.

**Size budget.** A snapshot is at most `kMaxMessageSize`. It is filled in this order: despawns, spawns, then
updates, the oldest-confirmed entities first. An entity is included whole or not at all. Whatever does not
fit goes in the next snapshot: it is still unconfirmed, so it is selected again automatically. Nothing is
lost, and nothing starves.

**Client side:**

1. A snapshot whose tick is not newer than the last applied one is dropped.
2. Otherwise it is applied in order: spawns (create the entity if it does not exist, then set its
   components), updates (set, adding the component if missing), removals, despawns.
3. `kEntity` fields are resolved after the spawns, so they can refer to entities spawned by the same
   snapshot.
4. Components that are interpolated are written to their `Interpolated` buffer instead of being displayed
   directly.

**Interpolation.** The client keeps a display clock about 100 ms behind the newest snapshot tick received,
advanced every frame by the frame time and gently corrected when it drifts. Entities are drawn
interpolated between the two snapshots around that time. Ticks, not arrival times, give the spacing between
snapshots, so network jitter does not make motion speed up and slow down.

**Snapshot rate:** one snapshot per server tick (60 Hz) to begin with; see [Open questions](#16-open-questions).

### 7.5 Inputs (client → server)

**Layout:**

```
u8   type = kInput
u32  last applied snapshot tick        the confirmation of 7.4
u8   input count (1 to 3)
     input × { sequence u32, buttons u32, axes i16 × n }
```

- `buttons` has one bit per `kButton` action, in manifest order; bits beyond the number of button actions
  must be 0. `axes` has one `i16` per `kAxis` action and two per `kVector2` action, in manifest order, mapping
  [-1, 1] to [-32767, 32767].
- `sequence` is the client's own input counter, incremented once per fixed tick on the client.
- **Each message carries the last 3 inputs.** If one datagram is lost, the next one still contains its
  input, with no extra delay. Resending reliably would not help: the resend would arrive after that tick is
  over. Three inputs cost about 24 bytes.
- Inputs are **held states**, not events. The server computes "pressed this tick" by comparing an input with
  the previous one, so a lost packet can never swallow a press that is still held.

**Server side:**

1. Inputs are kept in a small buffer per client, by sequence; sequences already applied are ignored.
2. Every `FixedUpdate`, `net.applyInputs` takes the next input of each client and writes it to the
   `PlayerInput` of the entity with that client's `Owner`. If none arrived, the last one is repeated.
3. If the buffer grows beyond 4 inputs (the client runs slightly fast or sent a burst), the oldest are
   dropped down to 2.

## 8. Custom packets

Custom packets are game-defined messages for what is not state: "player X was killed by Y" for a kill feed,
"ready" in a lobby, a chat line. They are **events, not commands**: a client-to-server packet expresses an
intention that the server checks, never a state that the server takes as true.

### 8.1 Declaring a packet

A packet is declared with a **schema table**: an ordered array of fields. The same type names are used as
for components.

```luau
-- shared/packets/PlayerKilled.luau, loaded by the server and the client
return net.packet("game.PlayerKilled", {
    direction = "toClients",   -- "toClients" or "toServer"
    channel = "reliable",      -- "reliable" or "unreliable"
    fields = {
        { name = "victim", type = "Entity" },
        { name = "killer", type = "Entity" },
    },
})
```

| Type | On the wire | Checked on receive |
|---|---|---|
| `bool` | `u8` | 0 or 1 |
| `i8` `i16` `i32` `i64` `u8` `u16` `u32` `u64` | little-endian integer of that size | |
| `f32` `f64` | IEEE 754, little-endian | finite (no NaN, no infinity) |
| `vec2` `vec3` `vec4` | 2, 3 or 4 × `f32` | each component finite |
| `Entity` | `NetworkId` `u32`, 0 = none | an unknown id becomes the null `Entity` |
| `AssetId` | `u64` | |
| `string` | length `u8`, then UTF-8 bytes | length ≤ the field's `maxLength` (default 64, at most 255) |

Rules:

- **Field order is the array order**, so it is the same on every machine.
- Packets are declared only at startup. Once the server has built its manifest, or once the client has
  connected, `net.packet` fails.
- The field name `sender` is reserved ([8.3](#83-receiving)).
- Luau numbers are doubles, so `i64` / `u64` values beyond ±2^53 cannot be represented: sending one is an
  error.

**Schema hash.** It hashes the packet's name, direction, channel and every field (name, type,
`maxLength`) in order, with the 64-bit FNV-1a of the ECS (`hashString` / `hashInteger` in
[`NameHash.hpp`](../../src/rtype/ecs/component/NameHash.hpp)). It goes in the manifest: a client whose
script declares the packet differently is refused at connection, instead of silently misreading every
packet after it.

**Wire size of `PlayerKilled`:** type `1` + short id `2` + victim `4` + killer `4` = **11 bytes**, plus the
3-byte message header on the reliable channel (and its 2-byte id).

C++ packets use the same descriptor (name, direction, channel, fields) and the same serializer; how C++
structs declare their fields is an [open question](#16-open-questions).

### 8.2 Sending

```luau
-- server system: gameplay decided that the ship died
ctx.net:sendToRoom("game.PlayerKilled", { victim = ship, killer = enemy })
```

| Function | Side |
|---|---|
| `ctx.net:sendToRoom(name, data)` | server: every client of the room |
| `ctx.net:sendTo(client, name, data)` | server: one client |
| `ctx.net:sendToServer(name, data)` | client |

Sending checks every value against its type and raises a script error on a mismatch: a missing field,
`300` or `1.5` into a `u8`, a NaN, a string that is too long, or a packet sent in the wrong direction.

### 8.3 Receiving

Every packet type is registered as an **ECS event** with the packet's name. `net.receive` writes the
received packets into that event queue, and systems read them like any other event, inside the schedule:

```luau
local KillFeed = ecs.system("game.KillFeed", { side = "client", events = { "game.PlayerKilled" } })

function KillFeed:run(ctx)
    for packet in ctx.events["game.PlayerKilled"] do
        showKillFeed(packet.victim, packet.killer)
    end
end

return KillFeed
```

On the server, every event from a `toServer` packet also carries `sender`, the client id it came from.

A received packet is dropped if:
- its short id is unknown;
- it comes in the wrong direction or on the wrong channel;
- a field fails its check (see the table in [8.1](#81-declaring-a-packet));
- bytes are missing or left over.

Every drop counts toward the sender's error budget ([10](#10-robustness-and-security)).

### 8.4 Shooting is an input, not a packet

A packet like `PlayerShoot { origin = ... }` sent by the client would let a cheater spawn bullets anywhere.
Shooting is an **input action**: the client only says "fire is held", and the server decides where the
bullet appears.

```luau
-- the game's plugin
app:inputAction("fire", "button", { key = "Space", gamepad = "South" })
```

```luau
-- server system (illustrative: the PlayerInput Luau API belongs to the Luau bridge)
function Shoot:run(ctx)
    for entity, transform, input in ctx.query do
        if input:isHeld("fire") then
            ctx.commands:spawnPrefab("game.Bullet", { ["rtype.Transform"] = { position = transform.position } })
        end
    end
end
```

The bullet then reaches every client as a spawn in the next snapshot, which is how "a player shoots" is
notified to the others (N1).

## 9. Lobby and rooms

After `kReady`, a client is in the **lobby**: connected, but in no room. The lobby is handled by the I/O
thread with these reliable messages:

| Value | Message | Direction | Content |
|---|---|---|---|
| `0x01` | `kRoomListRequest` | C → S | |
| `0x02` | `kRoomList` | S → C | count `u8`, count × { room id `u32`, players `u8`, max players `u8`, open `u8` } |
| `0x03` | `kCreateRoom` | C → S | |
| `0x04` | `kJoinRoom` | C → S | room id `u32` |
| `0x05` | `kLeaveRoom` | C → S | |
| `0x06` | `kRoomJoined` | S → C | room id `u32` |
| `0x07` | `kRoomError` | S → C | reason `u8` (`kFull`, `kNotFound`, `kClosed`, `kTooManyRooms`) |
| `0x08` | `kClientLeft` | S → C | client id `u32`, reason `u8` |
| `0x09` | `kReady` | C → S | (sent once, after the manifest is checked) |

- **Joining:** the I/O thread checks the room's capacity, sets `Session::roomId`, pushes `ClientJoined` to
  the room's inbox and answers `kRoomJoined`. The room's `net.receive` turns `ClientJoined` into the ECS
  event `ClientConnected`; the game spawns the player's ship, and the client starts receiving snapshots
  (everything is a spawn for it at first, which is the full snapshot).
- **Creating:** the I/O thread forwards the request to the `RoomManager` on the main thread, which creates
  the room's `App` and thread and registers its inbox and outbox with the I/O thread. The creator is then
  joined to it.
- **Open or closed:** a room can close itself to new players (a game in progress) through an outbox command.
  `open` in `kRoomList` reflects it.
- **Leaving** (by `kLeaveRoom`, disconnect or timeout): `ClientLeft` goes to the room, which raises the ECS
  event `ClientDisconnected{clientId, reason}` so the game removes the ship, and `kClientLeft` goes to the
  other clients of the room. A room with no players left is stopped by the `RoomManager`.

## 10. Robustness and security

### Rules for every byte from the network

- Parse with `ByteReader`. Drop the whole datagram or message if the reader is invalid or bytes are left
  over.
- Check every enum value (packet kind, channel, message type, reasons, input action kinds).
- Floats must be finite. A single NaN position would poison the physics.
- Every count is capped: messages per datagram, entities per snapshot, inputs per message (3), string
  lengths, room counts.
- **Never `throw`, `assert` or crash on network input.** Drop it, count it, and log it at a limited rate (so
  logs cannot be flooded). Exceptions (in `network/exceptions/`) are only for local setup errors, such as a
  port that cannot be bound.

### Limits per client

| Limit | Initial value | On excess |
|---|---|---|
| Datagrams per second per session | 120, bursts of 20 | Dropped |
| Connect requests per second per IP | 10 | Dropped |
| Pending challenges | 256, 5 s expiry | Oldest dropped |
| Sessions | `kMaxClients` (64) | `kRejected{kServerFull}` |
| Pending reliable messages | 1024 | Disconnect `kOverflow` |
| Protocol errors (bad messages, bad packets) | 20 per 10 s | Disconnect `kProtocolError` |

All values are starting points, to tune with measurements.

### Windows `connection_reset`

On Windows, when the server sends a datagram to a client that is gone (crashed, closed), the client's
machine answers with an ICMP "port unreachable" message, and Windows reports it as an error on the server
socket's **next receive** (`WSAECONNRESET`, `asio::error::connection_reset`). A receive loop that stops on
errors would stop receiving for **every** client right after one of them crashes, which breaks N3 and N4.

The server does both:

```c++
#ifdef _WIN32
// A datagram sent to a closed port would make the next receive fail with WSAECONNRESET.
BOOL reportConnectionReset = FALSE;
DWORD bytesReturned = 0;
WSAIoctl(socket.native_handle(), SIO_UDP_CONNRESET, &reportConnectionReset, sizeof(reportConnectionReset),
         nullptr, 0, &bytesReturned, nullptr, nullptr);
#endif
```

and, on every platform, re-arms the receive after any error except `operation_aborted`.

### Security scope

The challenge and the session token protect against forged source addresses and amplification. **Encryption
and authentication are out of scope:** the session token protects against spoofing only, not against an
attacker who can read the traffic. Cheating is limited by design (the server only accepts inputs), not by
cryptography.

## 11. Bandwidth estimate

300 moving entities, of which 250 are bullets, 4 clients:

| Approach | Bytes per entity per snapshot | Snapshot | Upload, 20 Hz × 4 clients |
|---|---|---|---|
| Naive: `u32` net id and `u32` name hash per component, raw `Transform` (20 B) and `Velocity` (8 B), every entity every time | ~40 | ~12 KB (10 datagrams) | ~7.7 Mbit/s |
| This design: bullets `kSpawnOnly` (one ~20 B spawn each); the 50 others with `u32` net id, `u16` short id, positions quantized to `i16` | ~11 for the 50 | ~0.6 KB (1 datagram) | ~0.4 Mbit/s |

Most of the gain comes from `kSpawnOnly` and quantization, not from compression, which is why there is no
compression library.

## 12. Testing

- **Sans-IO core.** `network::Connection` (the per-peer state: sequences, acks, reliable queue, RTT) has no
  socket and no clock of its own: `receive(bytes, now)` and `buildPacket(now) → bytes`. Tests drive two
  `Connection`s through a **simulated link** (loss, duplication, reordering, delay) with a fake clock.
- **Unit tests** (`tests/network/`): `ByteWriter` / `ByteReader` bounds, `isNewer` across the wrap, ack bits,
  reliable delivery (exactly once and in order under 30 % loss), handshake (challenge, token, version,
  padding), timeouts.
- **Fuzz test:** random and truncated bytes fed to every parser must never crash, read out of bounds or
  throw.
- **Replication tests** (`tests/replication/`): two worlds connected through the simulated link; check that
  spawns, updates, removals and despawns converge under loss, that a snapshot over budget is completed over
  the next ones, and that a manifest mismatch is refused.
- **Integration:** a `server` and two `client` processes on localhost; kill a client and check that the
  other one receives `kClientLeft` and that the server keeps its tick rate.
- **Statistics** per session (RTT, loss, bytes per second in and out), readable by a debug overlay.

## 13. Differences with the ECS docs

This document changes or settles some points of the ECS docs, which are not edited here. To report there
later:

| ECS doc | It says | This document |
|---|---|---|
| [`20-replication.md`](../ecs/features/20-replication.md), packet layout | Header: protocol id `u16`, type `u8`, tick `u32`, ack `u32`, ack bits `u32` | Transport header of [6.2](#62-packets) (`u16` sequence and ack, session token); the tick moves into the snapshot |
| [`08-change-detection.md`](../ecs/features/08-change-detection.md), "Replication's own last run" | One acked tick per client | One confirmed tick **per client and per entity**, so a snapshot over budget can be completed later ([7.4](#74-snapshots-server--client)) |
| `20-replication.md`, update layout | Field mask per update | Whole components in v1, field masks later |
| `20-replication.md`, inputs | Inputs numbered by tick | Inputs numbered by a client input sequence; the message also carries the snapshot confirmation |
| `20-replication.md`, open questions | Transport details, packet size, encryption | Settled: [6](#6-transport-rtype-network), 1200 bytes, no encryption |
| `20-replication.md` / [`14-app-and-plugins.md`](../ecs/features/14-app-and-plugins.md), rooms | Rooms as separate Apps or one App with several worlds? | One `App` per room, each on its own thread |
| [`19-luau-bridge.md`](../ecs/features/19-luau-bridge.md), components from types | Luau components read from `export type` with `Luau.Ast` | AST parsing is dropped for now: packets use schema tables, and Luau components are expected to follow |
| [`11-events.md`](../ecs/features/11-events.md), open question | Should some events be replicated? | Custom packets are network events, read with `ctx.events` ([8.3](#83-receiving)) |
| `CLAUDE.md`, project structure | `engine` is the executable | `engine` becomes a library; `client` and `server` are the executables |

## 14. Pitfalls

1. **Sending `Entity` values.** They are local slots: entity 15 on the server is something else on the
   client. Send `NetworkId` and translate it.
2. **Ids from `typeid` or registration order.** They differ between compilers and processes. Use names and
   the manifest.
3. **Server logic in client systems.** Use the system side rule (`side = "server"`) of
   [`12-systems.md`](../ecs/features/12-systems.md), not "am I the server?" checks in scripts.
4. **Displaying positions as they arrive.** Snapshots at 20–60 Hz look jerky on a 144 Hz screen. Interpolate
   ([7.4](#74-snapshots-server--client)).
5. **Datagrams over 1200 bytes.** The OS fragments them, and one lost fragment loses the whole datagram.
6. **Touching a world from the I/O thread, or a session from a room thread.** Data races. Only queues cross
   threads.
7. **Comparing sequence numbers with `<`.** The connection dies after 18 minutes. Use `isNewer`.
8. **Stopping the receive loop on an error.** On Windows, one crashed client would then stop the server from
   receiving anything.
9. **Trusting client data.** Positions, origins or damage values from a client are cheats waiting to happen.
   Clients send inputs and intentions only.
10. **Sending differences instead of states.** "Health −10" breaks when a packet is lost or duplicated;
    "health = 40" does not.

## 15. Build order

1. `network`: `ByteWriter` / `ByteReader`, `isNewer`, `Connection` (acks, reliable channel), tested
   through the simulated link.
2. Socket and handshake: challenge, token, timeouts. Split `engine` into a library and add the `client` and
   `server` executables; two processes exchange messages.
3. `replication`: manifest, then snapshots with spawns only: the client sees the server world.
4. Inputs up: the ship moves on the server and on the client.
5. Updates, removals, despawns, per-entity confirmations, size budget, `kSpawnOnly`: scales to many bullets.
6. Lobby and rooms, custom packets, interpolation, quantization.

## 16. Open questions

| Question | Owner |
|---|---|
| Library kind of `network` and `replication`: shared like `ecs` and `luau`, or static? | Team |
| Snapshot rate: 60 Hz (every tick) or 30 Hz, from bandwidth measurements | Team |
| The local player's ship waits RTT + interpolation delay (100 ms or more): draw it from the newest snapshot without delay? Client prediction for it later? | Team |
| How C++ structs declare packet fields (a macro like `RTYPE_ECS_COMPONENT`?) | Network + ECS |
| Where the Luau `net` bindings (`net.packet`, `ctx.net`) live: in `replication` (which would then depend on `luau`) or in the engine's script plugin | Network + Charles |
| Maximum players per room (4?) and maximum rooms | Team |
| Can different rooms run different games (different manifests)? v1 assumes one manifest for the server. | Team |
| Timeout, resend and rate-limit values, to tune with the statistics of [12](#12-testing) | Network |
| Does the subject require a written protocol description (RFC style) beyond this document? | Team |

## 17. References

- Glenn Fiedler, Gaffer On Games: [Reliability, Ordering and Congestion Avoidance over UDP](https://gafferongames.com/post/reliability_ordering_and_congestion_avoidance_over_udp/),
  [Reliable Ordered Messages](https://gafferongames.com/post/reliable_ordered_messages/),
  [Client Server Connection](https://gafferongames.com/post/client_server_connection/)
- [netcode standard](https://github.com/mas-bandwidth/netcode/blob/main/STANDARD.md): challenge, tokens and
  anti-amplification padding
- Fabien Sanglard, [Quake 3 network model](https://fabiensanglard.net/quake3/network.php): snapshots
  relative to the last acknowledged state
- [bevy_replicon](https://github.com/projectharmonia/bevy_replicon): ECS replication with per-client,
  per-entity change ticks
- Valve, [Source Multiplayer Networking](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking):
  interpolation delay and input handling
