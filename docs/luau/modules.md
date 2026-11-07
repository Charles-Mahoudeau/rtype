# Luau module loader

`rtype::luau::ModuleLoader` gives scripts `require(path)`. It is free of the engine: it knows no file, folder, game or
plugin. The host tells it how a path becomes source code; the loader does the rest. Nothing in the engine uses it yet, so
a game script cannot see it (`ScriptHost` will, see the epic "[Luau] require() for game scripts").

## Wiring

```cpp
ModuleLoader loader{runtime,
                    [](std::string_view requesterKey, std::string_view path) -> Result<ModuleSource> { ... },
                    [](std::string_view key, bool entering) { ... }};  // optional run hook
auto value = loader.require("main");  // a root require, from C++
```

- `ModuleSource { key, chunkName, source }`: the `key` is the identity of a module (two paths with the same key are one
  module), the `chunkName` is what errors show (`<chunkName>:<line>:`), `source` is the code.
- The **resolver** gets the key of the module that asks (empty for a root require) and the path. On failure its error
  message is the reason the script sees.
- The **run hook** is called with `entering = true` before a module runs and `false` after, also when it fails, nested
  like the requires. A cache hit runs nothing and calls nothing. The host uses it to track who owns what a module declares.
- The runtime must outlive the loader, and the loader cannot be moved. A `require` called after the loader is destroyed
  raises a Luau error (it never dangles).

## What a module is

Each module is compiled by `Runtime::load` (so it runs in its own thread) and run once. It sees a `require` function
bound to its own key: that is how the resolver knows the requester without debug information, and why
`pcall(require, "x")` works.

**Environment.** A sandboxed runtime (`Runtime::sandbox()`, see `sandbox.md`) already gives each thread its own
globals; the loader adds `require` to them. An unsandboxed runtime shares the global table between threads, so the
loader gives each module a table of its own that falls back on the shared globals (`__index`). Either way a module's
global writes and its `require` stay in the module, and the shared globals are untouched. Sandboxing remains the way to
make the shared globals and libraries read-only.

**Return value.** 0 values give `nil`, 1 value is the module's value, 2 or more is an error. A returned table is frozen
(shallow, like `table.freeze`); other values pass through. The value is cached by key: a module runs once, and every
require gets the same instance. A root require from C++ follows the same rules; a module that returns nothing is fine
there (`Ref::isNil()`).

## Errors

A `require` from Luau raises `<requester chunk>:<line>: require("<path>"): <reason>` (the prefix comes from
`luaL_where`, so it is empty when `pcall(require, ...)` calls it directly). A root require gives
`require("<path>"): <reason>` as a `kRuntime` failure. Reasons:

| Reason | When |
| --- | --- |
| the resolver's message | the path does not resolve |
| `cycle: a.luau -> b.luau -> a.luau` | the module is being loaded: the whole loading stack, by chunk name |
| `returned N values` | N is 2 or more |
| `failed: <inner error>` | the module did not compile or raised an error |
| `'<chunk>' is not loaded: require it at the top of a module` | a runtime require (below) missed the cache |

A non-string argument raises `require expects a string path, got <type>`. A failed module is not cached, and its edges
are dropped: a later require retries. A Luau error never crosses into C++ raw.

## Loading versus runtime

A require may **load** a module only while a load is in progress: during a root `require()` (and so every require of
the modules it runs) or between `begin()` and `commit()`/`rollback()`. Otherwise (a system calling `require` later) it
is served from the cache and a miss fails with the "is not loaded" reason.

## Graph and reload

Every successful require records an edge requester to required, cache hits included.

- `dependents(keys)`: the keys and every module depending on them, directly or not, in topological order (dependencies
  first, ties broken by key, so it is deterministic).
- `reachableFrom(roots)`: the roots and everything they require, sorted.
- `isLoaded(key)`, `value(key)`, `loadedKeys()`.

A hot reload is a transaction:

1. `begin()`.
2. `invalidate(dependents(changed))` moves those entries (and their own edges) out of the cache. `Ref` is move-only, so
   they are kept aside, not copied.
3. Require the entries again: modules load because a transaction is active.
4. `commit()` drops what was set aside, or `rollback()` restores the cache and the graph exactly as they were at
   `begin()` (modules loaded during the transaction are forgotten).

`unload(keys)` removes modules that are no longer reachable; inside a transaction it behaves like `invalidate`, so a
rollback brings them back. Misuse (`begin` twice, `invalidate`/`commit`/`rollback` without a transaction) throws
`exceptions::InvalidOperation`.
