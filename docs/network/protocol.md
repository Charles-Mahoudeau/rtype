# AYLO Network Protocol, version 1

| | |
|---|---|
| **Status** | Implemented (`kProtocolVersion = 1`) |
| **Transport** | UDP, one port (default `4242`) |
| **Source of truth** | `src/rtype/network/protocol/`, `src/rtype/network/connection/Connection.cpp`, `src/rtype/replication/` |
| **Background** | [overview.md](overview.md) (the short story), [design.md](design.md) (the reasons) |

This document specifies the bytes a client and a server exchange, precisely enough to write a new client for an AYLO
server, in any language, without reading the engine's code. Where this document and the code disagree, the code is
right and this document has a bug: please open an issue.

The key words **MUST**, **MUST NOT**, **SHOULD** and **MAY** are to be read as in
[RFC 2119](https://www.rfc-editor.org/rfc/rfc2119).

## 1. Overview

The protocol has two layers:

1. **Transport** (section 3 to 6): the handshake, sessions, numbered datagrams, acknowledgements and a reliable,
   ordered channel next to an unreliable one. It carries opaque **messages**.
2. **Replication** (section 7 to 11): the messages themselves. The server describes the game (the **manifest**), then
   sends the state of the world (**snapshots**); the client sends its held buttons and axes (**inputs**); both sides
   may send game-defined **custom packets**.

The server is authoritative: a client never sends world state, only inputs and the custom packets the game allows.

```
Client                                          Server
  │ ConnectRequest(version, resume?)  ──────────► │
  │ ◄──────────────────────────  Challenge(nonce) │
  │ ChallengeResponse(nonce)  ──────────────────► │
  │ ◄───────  Accepted(session, client, resume)   │
  │ ◄═══════  Data: manifest (reliable)           │
  │ Data: Ready (reliable)  ════════════════════► │
  │ ◄───────  Data: snapshots (unreliable, ~60/s) │
  │ Data: inputs (unreliable, every frame) ─────► │
  │ ◄══════►  Data: custom packets (either)       │
  │ Disconnect  ────────────────────────────────► │
```

## 2. Conventions

- Every multi-byte integer is **little-endian**. Floats are IEEE 754 binary32 (`f32`) or binary64 (`f64`), little-endian.
- `u8`, `u16`, `u32`, `u64` are unsigned; `i8`, `i16`, `i32`, `i64` are two's complement.
- There is **no padding and no alignment** between fields: a structure is its fields, one after the other.
- `string8` is a `u8` length followed by that many bytes of UTF-8 (no terminator).
- A datagram is at most **1200 bytes** (`kMaxPacketSize`). A sender MUST NOT send a bigger one; a receiver MUST drop it.
- A receiver MUST drop, without answering, any datagram it cannot parse entirely: unknown kind, missing bytes, bytes
  left over where none are allowed, an enum value outside its range, a non-finite float. A malformed datagram never
  closes a connection by itself.

## 3. Datagrams

The first byte of every datagram is its **kind**:

| Kind | Value | Direction | Section |
|---|---|---|---|
| `ConnectRequest` | `0x01` | C → S | 4.1 |
| `Challenge` | `0x02` | S → C | 4.2 |
| `ChallengeResponse` | `0x03` | C → S | 4.3 |
| `Accepted` | `0x04` | S → C | 4.4 |
| `Rejected` | `0x05` | S → C | 4.5 |
| `Data` | `0x06` | both | 5 |
| `Disconnect` | `0x07` | both | 4.6 |

`0x00` and `0x08` to `0xFF` are invalid.

## 4. Handshake and sessions

### 4.1 ConnectRequest (C → S), 64 bytes

| Offset | Size | Field | Value |
|---|---|---|---|
| 0 | 1 | kind | `0x01` |
| 1 | 4 | protocolId | `0x52545950` ("RTYP", written little-endian: `50 59 54 52`) |
| 5 | 2 | version | `1` |
| 7 | 4 | resumeClient | `0` for a new session, or the client id to resume (4.7) |
| 11 | 8 | resumeToken | `0`, or the resume token of that client |
| 19 | 45 | padding | zeros, up to 64 bytes in all |

The padding makes the request at least as large as any answer, so the server cannot be used to amplify traffic towards
a spoofed address. A server MUST drop a request shorter or longer than 64 bytes, or with another `protocolId`.

A server answers with `Rejected(kVersionMismatch)` when `version` is not its own, `Rejected(kServerFull)` when it has no
free slot, and otherwise with a `Challenge`. A client SHOULD resend its request every 100 ms (`handshakeRetry`) until it
gets an answer, and give up after 5 s (`handshakeTimeout`).

### 4.2 Challenge (S → C), 9 bytes

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | kind `0x02` |
| 1 | 8 | challenge: a random `u64` |

Only the real owner of the source address receives it. The server keeps a pending challenge for 5 s
(`challengeExpiry`) and at most 256 of them (`maxPendingChallenges`). A repeated `ConnectRequest` from the same address
gets the same challenge again.

### 4.3 ChallengeResponse (C → S), 64 bytes

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | kind `0x03` |
| 1 | 8 | challenge: the value received, unchanged |
| 9 | 55 | padding: zeros |

### 4.4 Accepted (S → C), 17 bytes

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | kind `0x04` |
| 1 | 4 | sessionToken: a random non-zero `u32`, carried by every later `Data` and `Disconnect` |
| 5 | 4 | client: the client id, from 1 upwards, never reused while the server runs |
| 9 | 8 | resumeToken: a random `u64`, secret, to resume this session after a loss (4.7) |

From now on the client is **connected**. The server immediately queues the manifest (section 8) on the reliable channel.

### 4.5 Rejected (S → C), 2 bytes

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | kind `0x05` |
| 1 | 1 | reason |

| Reason | Value | Meaning |
|---|---|---|
| `kVersionMismatch` | 1 | The protocol versions differ |
| `kServerFull` | 2 | No free slot (`maxClients`, 64 by default) |
| `kSessionExpired` | 3 | The session to resume no longer exists, or the resume token is wrong |

### 4.6 Disconnect (both ways), 6 bytes

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | kind `0x07` |
| 1 | 4 | sessionToken |
| 5 | 1 | reason |

| Reason | Value | Meaning |
|---|---|---|
| `kGraceful` | 1 | The peer closes on purpose |
| `kTimeout` | 2 | Nothing arrived for `timeout` (2 s) |
| `kKicked` | 3 | Removed by the server |
| `kProtocolError` | 4 | The peer broke the protocol (for instance a reliable id too far ahead) |
| `kManifestMismatch` | 5 | The client's game differs from the server's (section 8) |
| `kOverflow` | 6 | Too many reliable messages waiting to be sent |
| `kServerShutdown` | 7 | The server stops |

Nothing acknowledges a `Disconnect`, so a sender SHOULD send it 3 times. A `Disconnect` whose token is not the session's
MUST be ignored.

### 4.7 Timeouts and resuming

- Each side sends at least one datagram every 100 ms (`keepalive`): an empty `Data` (no messages) when it has nothing
  to say.
- A side that receives nothing valid for 2 s (`timeout`) considers the connection lost.
- A client that lost its connection MAY resume its session: it sends a `ConnectRequest` with `resumeClient` and
  `resumeToken` set, completes the challenge as usual, and gets a new `Accepted` with **new** session and resume
  tokens; the old ones are dead. The server sends the manifest again, then the whole world as spawns. If the session no
  longer exists the answer is `Rejected(kSessionExpired)`, and the client MAY start over with a new session.

## 5. Data datagrams

Once connected, everything travels in `Data` datagrams.

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | kind `0x06` |
| 1 | 4 | sessionToken |
| 5 | 2 | sequence: this datagram's number, +1 per datagram, wrapping from 65535 to 0 |
| 7 | 2 | ack: the most recent sequence received from the peer |
| 9 | 4 | ackBits: bit `n` set means sequence `ack - 1 - n` was received too (n = 0 to 31) |
| 13 | … | messages, up to the end of the datagram |

The header is 13 bytes. A receiver MUST drop a `Data` whose `sessionToken` does not match the session of its source
address. Before anything was received, `ack` and `ackBits` are 0.

**Sequence order.** Sequence `a` is newer than `b` when `(a - b) mod 65536` is between 1 and 32767.

**Acknowledgements.** Each datagram acknowledges the last 33 datagrams received (`ack` and 32 bits). A sender learns
that a datagram arrived when a later datagram from the peer acknowledges it; it never learns that a datagram was lost
for sure. Acknowledgements also measure the round-trip time.

### 5.1 Messages

The message area is a sequence of messages, each with a small header:

| Channel | Header | Then |
|---|---|---|
| Unreliable | `u8 channel = 0`, `u16 length` | `length` bytes of payload |
| Reliable | `u8 channel = 1`, `u16 id`, `u16 length` | `length` bytes of payload |

A payload is at most 1000 bytes (`kMaxMessageSize`); a message is never split over several datagrams. The first byte
of a payload is its **message type** (section 6).

**Unreliable** messages may be lost, duplicated or reordered: they are used for state that a newer message replaces.

**Reliable** messages arrive exactly once, in `id` order:

- The sender numbers its reliable messages from 0, +1 each, wrapping at 65535.
- It puts a message in a datagram, and puts it again in a later datagram if no datagram carrying it was acknowledged
  after about one round trip (at least 50 ms, `minResendInterval`).
- At most 256 reliable messages may be unacknowledged at once (`kReliableWindow`). If more than 1024 are waiting to be
  sent, the connection closes with `kOverflow`.
- The receiver delivers messages in `id` order, buffers the ones that arrive early, and drops an `id` it already
  delivered. An `id` 256 or more ahead of the next expected one is a protocol error.

A sender puts reliable messages first in a datagram, then unreliable ones, and sends at most 8 datagrams per flush.

## 6. Message types

| Type | Value | Direction | Channel | Section |
|---|---|---|---|---|
| `Ready` | `0x09` | C → S | reliable | 7 |
| `ManifestComponent` | `0x20` | S → C | reliable | 8.1 |
| `ManifestInputAction` | `0x23` | S → C | reliable | 8.2 |
| `ManifestEnd` | `0x24` | S → C | reliable | 8.3 |
| `Snapshot` | `0x30` | S → C | unreliable | 9 |
| `Input` | `0x31` | C → S | unreliable | 10 |
| `CustomPacket` | `0x32` | both | as declared | 11 |

Values below `0x20` belong to the transport. `0x21` and `0x22` are reserved (packets and capsules in the manifest, not
sent in version 1). A server ignores a game message from a client that is not `Ready` yet.

## 7. Joining

1. The server sends the manifest: every `ManifestComponent`, then every `ManifestInputAction`, then one `ManifestEnd`.
2. The client compares it with its own copy of the game (section 8.4). If they differ, it MUST send
   `Disconnect(kManifestMismatch)`.
3. Otherwise it sends `Ready`: a reliable message of one byte, `0x09`.
4. The server then starts sending snapshots to the client, and accepts its inputs and custom packets. The client
   ignores unreliable messages until the manifest is complete.

## 8. Manifest

The manifest gives a short id to every replicated component and an index to every input action. Both are positions in
a list **sorted by name**, so both sides compute the same ones.

### 8.1 ManifestComponent

| Size | Field |
|---|---|
| 1 | type `0x20` |
| 2 | shortId: the component's position, from 0 |
| 1 + n | name (`string8`), for instance `rtype.Transform` or `game.Health` |
| 8 | wireHash: identifies the component's layout (section 8.4) |

### 8.2 ManifestInputAction

| Size | Field |
|---|---|
| 1 | type `0x23` |
| 1 | index: the action's position, from 0 |
| 1 + n | name (`string8`), for instance `move` or `shoot` |
| 1 | kind: `1` button, `2` axis, `3` two-dimensional vector |

### 8.3 ManifestEnd

| Size | Field |
|---|---|
| 1 | type `0x24` |
| 2 | entryCount: the components and actions sent before it |
| 8 | hash: the manifest hash (section 8.4) |
| 1 | tickRate: simulation ticks per second |
| 1 | snapshotRate: snapshots per second |
| 2 | reconnectionGraceSeconds: how long a lost session waits to be resumed |

### 8.4 Agreeing on the game

A third-party client MUST know the game it plays: the fields of each replicated component and the input actions. They
are declared by the game's Luau scripts (`engine.ecs.component`, `engine.input.action`), not sent over the network.

The **manifest hash** is FNV-1a 64-bit (offset basis `14695981039346656037`, prime `1099511628211`), fed with, in order:

- for each component, by short id: `0x20`, `shortId`, the name's length, the name's bytes, `wireHash`;
- for each action, by index: `0x23`, `index`, the name's length, the name's bytes, `kind`.

Integers are fed as 8 bytes, lowest first; the name as its bytes. A client that computes its own manifest hash compares
it with `ManifestEnd.hash`. A simpler client MAY instead trust the names and short ids it received, at its own risk:
the `wireHash` of a component is opaque (a hash of its engine-side layout), and a client that does not compute it
cannot detect that a component changed.

## 9. Snapshot (S → C)

A snapshot is the state of the world for one client, about 60 times per second (`snapshotRate`). It carries **states,
never differences**: the server repeats each change in later snapshots until the client acknowledges a snapshot that
carried it (section 10, `lastAppliedSnapshot`). Applying the same snapshot twice is harmless.

| Size | Field |
|---|---|
| 1 | type `0x30` |
| 4 | tick: the server tick of this snapshot |
| 4 | lastProcessedInput: the sequence of this client's last input the server applied, 0 if none |
| 1 | flags: bit 0 `complete` (every replicated entity is listed, so the client removes the ones absent); bit 1 `part` (one of several messages of this tick); other bits MUST be 0 |
| 0 or 2 | only with `part`: `u8` index of this part, then `u8` number of parts; the index MUST be below the number, and the number at least 2 |
| 2 + … | spawns: `u16` count, then spawn records |
| 2 + … | updates: `u16` count, then update records |
| 2 + … | removals: `u16` count, then removal records |
| 2 + … | despawns: `u16` count, then `u32` network ids |

| Record | Layout |
|---|---|
| Spawn | `u32 networkId`, `u16 capsule` (`0xFFFF` for none), component list |
| Update | `u32 networkId`, component list |
| Removal | `u32 networkId`, `u16 shortId` |
| Component list | `u8 count`, then for each component: `u16 shortId`, then its value (9.1) |

- A network id is never 0, and appears at most once per list. Spawns are in increasing network id; a record lists its
  components in increasing short id, each once.
- **Spawn**: create the entity with these components. **Update**: set these components. **Removal**: remove one
  component. **Despawn**: destroy the entity.
- A snapshot whose `tick` is not newer than the last one applied MUST be ignored (except the other parts of that tick).
- A snapshot that does not fit in one message is split by entity into **parts**: messages of the same `tick`, each
  with the `part` flag, its index and the number of parts, and the same `complete` flag. An entity is never split
  across parts. A client applies each part as it comes; for a `complete` snapshot, it removes the components a part's
  records do not list at once, but despawns the entities no part listed only once every part of the tick is in. A part
  of a tick older than the last applied, or a part already applied, MUST be ignored; the first part of a newer tick
  abandons the parts of an older one, which then despawns nothing.

### 9.1 Component values

A value is its fields in declaration order, each field `count` times (an array), with no padding:

| Field type | Wire |
|---|---|
| `bool` | `u8`, 0 or 1 (anything else: drop the snapshot) |
| `i8` … `i64`, `u8` … `u64` | the integer |
| `f32`, `f64` | the float; NaN and infinities are invalid |
| `vec2`, `vec3`, `vec4` | 2, 3 or 4 `f32` |
| `Entity` | `u32` network id, 0 for none or an entity the client does not know |
| `AssetId` | `u64` |
| `string` | `string8` |
| a list (`text`, or a `{ type }` list) | `u16` count, then the values |

A component with a fixed-size layout has the same size in every snapshot. A component with a string or a list is
**length-prefixed**: its value is preceded by a `u16` byte length.

## 10. Input (C → S)

The client sends what is **held** now, never events or positions, every frame. Each message repeats the previous
inputs, so a lost datagram costs nothing.

| Size | Field |
|---|---|
| 1 | type `0x31` |
| 4 | lastAppliedSnapshot: the tick of the last snapshot the client applied (acknowledges it, section 9) |
| 1 | count: 1 to 3 |
| … | `count` samples, oldest first, sequences strictly increasing |

| Size | Sample field |
|---|---|
| 4 | sequence: the client's own counter, +1 per sample |
| 4 | buttons: bit `i` set when the button action of bit `i` is held |
| 2 × slots | axes: one `i16` per axis slot, the value in [-1, 1] times 32767, rounded |

Button bits and axis slots come from the manifest's actions, in index order: the `i`-th button action is bit `i`, an
axis action takes the next slot, a vector action the next two. A bit beyond the declared buttons makes the message invalid. The server applies one
sample per tick and repeats the last one when none arrived.

## 11. CustomPacket (both ways)

Packets the game declares in Luau (`net.packet`, see the manual's
[Networking](https://charles-mahoudeau.github.io/rtype/networking/) page), for what is not state: a chat line, a "ready"
button, a kill feed.

| Size | Field |
|---|---|
| 1 | type `0x32` |
| 1 + n | name (`string8`), at most 64 bytes |
| 4 | schema hash (below) |
| … | each field in declaration order |

| Field type | Wire |
|---|---|
| `bool` | `u8`, 0 or 1 |
| `u8`, `u16`, `u32`, `i32` | the integer |
| `f32` | the float, finite |
| `string` | `u16` length, then the bytes, at most the declared `maxLength` (64 by default, 1000 at most) |

The **schema hash** is FNV-1a 32-bit (offset `2166136261`, prime `16777619`) of: the name's bytes then a 0 byte, the
direction (`0` to the server, `1` to the clients), the channel (`0` unreliable, `1` reliable), then for each field its
name's bytes and a 0 byte, its type (`0` bool, `1` u8, `2` u16, `3` u32, `4` i32, `5` f32, `6` string) and its
`maxLength` as 2 bytes, lowest first. A receiver drops a packet whose name is unknown, whose hash differs, which comes
in the wrong direction or on the wrong channel, or whose values break the schema.

## 12. Security considerations

- **Spoofing.** The challenge proves the client receives at its address; the session token, carried by every later
  datagram, makes a datagram from another address useless. Tokens are random, and new on every resume.
- **Amplification.** Handshake requests are padded to 64 bytes, larger than any handshake answer.
- **Resource exhaustion.** Pending challenges, clients, reliable windows, queued messages, strings, lists and counts
  are all bounded; every count is checked against the bytes that remain before anything is allocated.
- **Malformed data.** Every read is bounds-checked; a malformed datagram or message is dropped and counted, it never
  crashes, throws or asserts.
- **Authority.** A client sends only inputs and the custom packets declared `toServer`, which the server validates.
- **Not covered.** There is no encryption: anyone on the path can read the traffic, and the tokens with it. The
  protocol prevents impersonation from elsewhere, not eavesdropping.

## 13. Writing a client: checklist

1. Send `ConnectRequest` (64 bytes) every 100 ms until `Challenge`; echo it in `ChallengeResponse` (64 bytes).
2. On `Accepted`, keep the session token, client id and resume token.
3. Implement `Data`: your sequence, the peer's `ack` and `ackBits`, and a keepalive every 100 ms.
4. Implement the reliable receiver (in order, no duplicates) and sender (resend until acknowledged).
5. Read the manifest; check it against your copy of the game; send `Ready` (`0x09`, reliable).
6. Apply snapshots by tick; send an `Input` every frame with your last applied tick and your last 3 samples.
7. On 2 s of silence, try to resume with your resume token; on `Disconnect`, stop.
