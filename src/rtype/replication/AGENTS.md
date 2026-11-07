# replication — world to messages and back

Turns a world into messages and back: component and event replication, snapshots, the manifest of replicated data,
input, and the server and client sides. Static library `replication`, linked into the executable. The only module that
knows both the ECS and the network.

## Boundaries

- Depends on `ecs` and `network`. The renderer, the platforms and `luau` never include its headers.
- Feature folders with sub-namespaces, as in `network`.

## Invariants

- The server is authoritative. A client never writes replicated data; it sends inputs, and the server decides.
- Messages from the network are untrusted: a bad or stale message is dropped, never a crash, an exception or an assert.
- Both sides agree on what is replicated through the manifest built from the data description in `ecs`; a plugin opts
  its data in, replication never special-cases a plugin.

## Tests and docs

- Tests: `tests/replication/` (`replication-tests`).
- Docs: `docs/network/design.md` and `docs/network/implementation.md` (replication tasks),
  `docs/ecs/features/20-replication.md`.
