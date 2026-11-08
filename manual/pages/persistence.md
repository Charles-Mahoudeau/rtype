# Persistence (saves)

The `rtype.Persistence` plugin saves named texts in a per-user folder. It does not look inside them: the game
serialises its table, checks its own `schema_version` and migrates.

## Setup

Add the plugin in `project.luau`. Optional `persistence` section:

| Key         | Default   | Meaning                                                                          |
| ----------- | --------- | -------------------------------------------------------------------------------- |
| `directory` | none      | Absolute folder for the saves. Without it: `<user data directory>/<folder>`.      |
| `folder`    | `"rtype"` | The game's folder in the user data directory (`%APPDATA%`, `~/Library/Application Support`, `$XDG_DATA_HOME` or `~/.local/share`). |

## From scripts

Each helper takes a system's `ctx` (listing the call under `calls`) or `nil` while the script loads, and a table.
A name is 1 to 64 letters, digits, `_`, `-` or `.`, not starting with a dot.

| Helper                                       | Call                      | Result                                  |
| -------------------------------------------- | ------------------------- | --------------------------------------- |
| `engine.persistence.read(ctx, { name })`     | `rtype.persistence.read`  | `{ ok, data, error }`                   |
| `engine.persistence.write(ctx, { name, data })` | `rtype.persistence.write` | `{ ok, error }`                      |
| `engine.persistence.delete(ctx, { name })`   | `rtype.persistence.delete`| `{ ok, error }`                         |
| `engine.persistence.exists(ctx, { name })`   | `rtype.persistence.exists`| `{ exists }`                            |

Failures are values, never script errors: `error` is `invalid_name`, `not_found`, `corrupt` (truncated or damaged
file) or `io: ...`. A write is atomic (temporary file, then rename): if it fails or the game crashes, the previous
save stays whole. Deleting a save that does not exist succeeds (use it for "reset save").

```lua
local loaded = engine.persistence.read(nil, { name = "profile" })
if not loaded.ok then
	-- first launch or damaged save: start from defaults
end
engine.persistence.write(nil, { name = "profile", data = serialised })
```

Saves live where the plugin runs: on a server-only game, add it to the server.
