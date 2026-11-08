# Custom packets (client ↔ server commands from Luau)

**Implemented.** The minimal slice of [design section 8](design.md#8-custom-packets) (tasks NET-33 to NET-35): typed,
validated messages that a script sends over the network, for what is not replicated state. Used for the commands of a
game (`buy`, `set_speed`, a free-form directive...). State still goes down as [replicated components](../ecs/features/20-replication.md).

## What goes where

| Direction | Mechanism | Limits |
|---|---|---|
| Server → clients, state | A component with `replication = "everyChange"` (or `"spawnOnly"` for append-only data such as a feed line), at the server's `snapshotRate` (60 Hz by default, `ReplicationSettings`; `10` gives a 10 Hz delta) | Typed fields; a `string` is at most 255 bytes |
| Client → server, commands | A `toServer` packet, `engine.net.sendToServer` | At most 1000 bytes per message (strings up to about 960 bytes) |
| Server → clients, one-shot | A `toClients` packet, `engine.net.sendToClients` | Same |

## Declaring a packet

Both sides load the same declaration (put it in a shared file). Declare while the scripts load: the registry is frozen
when the server or client starts, and a later declaration is an error.

```luau
engine.net.packet("game.Buy", {
    direction = "toServer",              -- or "toClients"
    channel = "reliable",                -- or "unreliable" (default "reliable")
    fields = { { "item", "string", 32 }, { "qty", "u8" } },  -- { name, type, maxLength? } in wire order
})
```

Types: `bool`, `u8`, `u16`, `u32`, `i32`, `f32`, `string` (`maxLength` defaults to 64, at most 1000). Field names are
identifiers, unique, and not `sender`. A packet whose largest message would not fit in the transport's 1000 bytes
(`6 + name + fields`, a string counting `2 + maxLength`) is refused at declaration, naming the packet.

## Sending, receiving, validating

```luau
-- client system: lists the call it makes
engine.ecs.system("game.Commands", {
    phase = "Update", side = "client", query = {}, calls = { "rtype.net.sendToServer" },
    run = function(ctx)
        engine.net.sendToServer(ctx, "game.Buy", { item = "laser", qty = 2 })   -- returns false when not connected
    end,
})

-- server system: reads what arrived, validates what it MEANS, answers
engine.ecs.system("game.Rules", {
    phase = "Update", side = "server", query = {}, calls = { "rtype.net.sendToClients" },
    events = { read = { "net.PacketReceived" } },
    run = function(ctx)
        for _, packet in engine.net.received(ctx) do      -- { name, sender (client id), values (a table) }
            if packet.name == "game.Buy" then
                local known = catalog[packet.values.item] ~= nil          -- tool exists? agent allowed? args valid?
                engine.net.sendToClients(ctx, "game.Ack", { ok = known, reason = known and "" or "unknown_item" }, packet.sender)
            end
        end
    end,
})
```

`engine.net.sendToClients(ctx, name, values, client?)` sends to every connected client, or to `client` (an id from
`net.ClientConnected` or a packet's `sender`); it returns `false` when nobody could get it.

A running example, with the three packets above, is the test `tests/scripting/ScriptPacketTests.cpp`.

## What the engine checks

| Where | Check | On failure |
|---|---|---|
| Sending | Packet declared, goes this way, every field present with the right type, integers whole and in range, `f32` finite, strings within `maxLength` | A Luau error naming the packet and the field; nothing is sent |
| Receiving | Message well formed (bounds-checked), packet known, direction, schema hash (the sender's declaration is the same), every value re-checked, no trailing bytes | Dropped; the scripts never see it |
| Receiving | At most 256 packets kept per frame (`ServerState::kMaxPendingPackets`) | The rest dropped, counted (`droppedPacketCount()`) |

The engine checks **shape**. Whether a command is allowed (the player owns that ship, has the gold, the tool exists)
is the game's rule: the server script decides, using `sender`. Nothing a client sends changes state by itself.

## Wire format

`kCustomPacket (0x32)`, `u8 nameLength`, `name`, `u32 schemaHash` (FNV-1a of name, direction, channel and fields), then
the fields in order: `bool`/`u8` 1 byte, `u16` 2, `u32`/`i32`/`f32` 4, `string` `u16 length` + bytes (little-endian).

Between a script and the engine, values travel as text (`<size>:<bytes>` per field), because Luau has no binary
packing: the engine parses and checks it again, so a script cannot send anything the schema forbids.

## Differences with the design

- **No manifest entry.** The design lists packets in the manifest; here the hash rides in every message, and a
  mismatch drops the packet silently (a client with other scripts is not refused at connection). A manifest check is
  still to do.
- **The name is on the wire**, not a 2-byte id.
- **No C++ `RTYPE_REPLICATION_PACKET` macro and no ECS event per packet**: all packets arrive as one event,
  `net.PacketReceived { name, sender, values }`, which `engine.net.received` turns into tables. `Entity` fields are not
  supported.
- **`PacketRegistry`** (`net.PacketRegistry`) holds the declarations; `rtype.net.declarePacket`,
  `rtype.net.sendToServer` (client side) and `rtype.net.sendToClients` (server side) are the engine calls behind the
  forms.
