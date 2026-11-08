# Luau sandbox and table freeze

How `rtype::luau::Runtime` isolates scripts from each other and from the standard libraries. Nothing in the engine
turns it on yet: a game script cannot see it (`ScriptHost` will call `sandbox()` once `require` lands, see the
epic "[Luau] require() for game scripts").

## Opt-in sandbox

A new `Runtime` is **not** sandboxed: every script shares one global table and may change the standard libraries. This
lets the host set the globals every script must see (the `engine` table, the forms, test fixtures) first.

`Runtime::sandbox()` then seals the environment (`luaL_sandbox`). It is idempotent, and `isSandboxed()` tells whether it
was called. After it:

| What | Behavior |
| --- | --- |
| The shared global table | Frozen. Globals set **before** `sandbox()` are seen by every script |
| Standard libraries (`math`, `string`, `table`...) | Read-only. A script writing `math.max = nil` gets a Luau error (`kRuntime`) |
| A script loaded by `load()` | Runs in its own thread environment (`luaL_sandboxthread`): the globals it assigns stay in it, and it still reads the shared globals |
| `Runtime::globals()` | Still returns the shared table, now frozen |
| `globals().set(...)` after sealing | Returns a `kRuntime` failure ("attempt to modify a readonly table"), like any write to a frozen table; it never throws or crashes |

Publishing a usertype constructor (a script global) after sealing throws `InvalidOperation`: register usertypes
before `sandbox()`.

Scripts loaded **before** `sandbox()` keep sharing the (now frozen) global table: seal before loading scripts.

## Table freeze

`Table::freeze()` makes a table read-only, shallowly like Luau's `table.freeze`: nested tables stay writable.
`isFrozen()` reads the flag. Freezing is irreversible. A write from a script raises a Luau error, and from C++ `set`,
`setAt`, `setMetatable` and `clearMetatable` return a `kRuntime` failure; the host does not crash.

One exception: assigning an **absent** key of a frozen table that has a `__newindex` metamethod calls the handler
instead of raising a read-only error (Luau only checks the flag when it writes the table itself). The handler can
return normally or have side effects; the frozen table stays unchanged.
