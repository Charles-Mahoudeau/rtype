# Storage

What the engine and its games keep on disk, in which format, and how it survives a crash. The engine has no database:
nothing it stores needs one yet. This page explains why, and what would change that.

## What is stored

| Data | Written by | Format | Where |
|---|---|---|---|
| Game settings | The game developer | Luau (`project.luau`), run at startup | The game folder |
| Scripts | The game developer | Luau sources | `<game>/scripts/` |
| Scenes, actors, levels (capsules) | The game developer, the editor | Our text format ([`CapsuleFormat.md`](../CapsuleFormat.md)) | `<game>/assets/` |
| Assets (images, sounds, fonts, shaders) | Artists | PNG, WAV/OGG/MP3, SDF font atlas + JSON, GLSL | `<game>/assets/`, referred to by a 64-bit id hashed from the path |
| Player saves (progress, options, key bindings) | The game, at run time | Text the game chooses, in a checked envelope | A per-user folder (`%APPDATA%`, `~/Library/Application Support`, `$XDG_DATA_HOME`) |
| Plugin signatures | `aylo-sign` | Ed25519 key, certificate and signature | `<library>.sig` next to each game plugin |

## Options compared

| Option | Persistence | Reliability | Constraints | Fit for us |
|---|---|---|---|---|
| **Plain files, one per save, atomic replace** | Survives restarts | A crash during a write leaves the previous file whole (write to a temporary file, then rename) | No query, no concurrent writers | **Chosen for saves**: a game saves a few kilobytes, rarely, from one process |
| SQLite | Survives restarts | Transactions, journal | One more dependency; a schema to migrate | Worth it for many records queried often (accounts, scoreboards, inventories) |
| Key-value store (LevelDB, RocksDB) | Survives restarts | Write-ahead log | Heavy build, more than we need | No |
| Remote database (PostgreSQL, a web API) | Shared between machines | Depends on the service | Needs a server, credentials, a network | Possible from a game through the HTTP plugin ([`Http.md`](../Http.md)) for global scoreboards; not engine-level |
| Binary blobs (raw structs) | Survives restarts | Breaks with any layout change, endianness, padding | Not editable, not diffable | No: our formats are text or checked |
| Standard formats for scenes (JSON, TOML, YAML) | – | – | Do not keep the file's order (most libraries), or add a dependency | Compared with TOML in [`../benchmarks/capsule-format.md`](../benchmarks/capsule-format.md): we kept our own format |

## How a save stays reliable

The persistence plugin ([manual page](https://charles-mahoudeau.github.io/rtype/persistence/)) stores each save as a
header and the game's text:

```
RTSAVE1
<size in bytes>
<FNV-1a 64-bit checksum in hex>
<exactly size bytes of the game's data>
```

- **Atomic writes.** The file is written to `<name>.tmp`, then renamed over the old one: a crash or a full disk never
  leaves half a save.
- **Corruption is detected.** A wrong magic, a size that does not match or a checksum that differs is reported as
  `corrupt`, a value the script handles (start from defaults), never a crash.
- **Names are checked.** 1 to 64 letters, digits, `_`, `-` or `.`, not starting with a dot: a script cannot write
  outside the save folder.
- **Versioning is the game's.** The engine does not look inside the data; the game stores a `schema_version` and
  migrates old saves.
- **Per user, per side.** Saves live in the user's data folder, on the side where the plugin runs: a networked game
  keeps them on the server, so a client cannot edit its own progress.

The checksum detects accidental damage, not tampering: a player can rewrite a local save and its checksum. Anything
that must not be cheated lives on the server.

## Scene files

Capsule files are text, so they are reviewed and merged in Git like code. The reader reports every error with the
file, line, column, component and field, and a scene that fails to load on a hot reload leaves the previous one
running. The writer keeps the file's order, so an editor's save changes only the lines that changed.

## Limits and next steps

- No accounts or global scoreboard: the server keeps nothing between runs but what its game saves. A multi-instance
  server with users would need a real database (SQLite on the server is the natural next step).
- Saves are not encrypted or signed: see [Security](security.md).
