# Luau API Proposal

This document proposes a C++-friendly wrapper around [Luau](https://luau.org/) for
the R-Type engine: enemy behaviors, level scripting, and other gameplay logic
that should be editable without recompiling the engine.

**Goals:** a builder-style registration API that reads naturally from C++, no
exceptions crossing the C++/Luau boundary (scripts are treated as untrusted,
frequently-changing content, so failures are recoverable `Result<T>` values,
not thrown exceptions), and low enough overhead to call into scripts every
frame for many entities at once.

**Non-goals**, given the project deadline — see [Non-Goals](#non-goals) for
the full list and rationale: exposing `os`/`io`, a `require()`-based module
system across files, a full debugger/breakpoint UI, and general-purpose
`buffer`/`vector` bindings beyond what gameplay code needs.

The sections below build on each other: [Error Handling](#error-handling) and
[Values & Tables](#values--tables) introduce the vocabulary types
(`Result<T>`, `Error`, `Value`, `Table`) used everywhere after.

## Error Handling

Every operation that can fail because of *script* content — a compile error,
a script raising an error, exceeding the memory or time budget — returns a
`rtype::luau::Result<T>` instead of throwing. Failures that come from
misusing the *C++* API instead (e.g. registering a function after
[`seal()`](#sandboxing), a duplicate usertype name) are programmer errors and
are asserted at registration time, not surfaced as `Result<T>`: they aren't
recoverable and aren't expected to happen outside of development.

```c++
enum class ErrorKind {
    kSyntaxError,    // script failed to compile (Runtime::load)
    kRuntimeError,   // script called error()/assert(), indexed nil, ...
    kTypeMismatch,   // a C++ binding received an argument of the wrong type
    kTimeout,        // exceeded RuntimeConfig::callBudget
    kOutOfMemory,    // exceeded RuntimeConfig::memoryLimit
    kStackOverflow,  // exceeded Luau's call stack depth
    kUnknown,
};

class Error {
public:
    ErrorKind kind() const;
    std::string_view message() const;   // human-readable description
    std::string_view traceback() const; // Luau "stack traceback:", empty if none
};

template <typename T>
class Result {
public:
    explicit operator bool() const; // true when holding a value
    const T& operator*() const;
    const T* operator->() const;
    const Error& error() const;     // only valid when operator bool() is false
};

// Result<void> specializes away operator*()/operator->().
```

`kTimeout` is enforced through Luau's interrupt callback (`lua_callbacks(L)->interrupt`),
which the runtime polls periodically during script execution — it is a
best-effort cooperative check, not a hard pre-emption, so a script that never
returns and never calls back into an interruptible point (e.g. a tight loop
doing pure arithmetic) is only interrupted at the next poll, not instantly.

## Basics

Registering a callable C++ function, compiling a script, and calling into it.

```c++
rtype::luau::Runtime rt;

// Simple function definition: argument types and the return type are
// deduced from the lambda's signature. `.names(...)` supplies the
// parameter names in order, since they can't be deduced from the lambda
// (only used for error messages and generateTypes(), see below).
rt.globals().function("cppHello")
    .bind([](std::string name) {
        return std::format("Hello {}, from C++!", name);
    })
    .names("name");

// Advanced function definition: use this when you need access to the raw
// call (variadic arguments, optional arguments, or manually raising a
// script error — see Call Context below).
rt.globals().function("cppHelloRaw")
    .arg<std::string>("name")
    .ret<std::string>()
    .handler([](rtype::luau::CallContext& ctx) {
        std::string name = ctx.get<std::string>("name");

        ctx.ret(std::format("Hello {}, from C++!", name));
    });

// Compiles the chunk named "cpp_hello.luau" with source code "src".
rtype::luau::Result<rtype::luau::Script> script = rt.load("cpp_hello.luau", src);

if (!script) {
    // Failed to compile (ErrorKind::kSyntaxError).
    return;
}

rtype::luau::Function* init = script->function("init");

if (!init) {
    // Missing "init" function at the top level of the script.
    return;
}

// Calling into a script can itself fail (runtime error, timeout, ...), so
// it returns a Result too.
rtype::luau::Result<rtype::luau::Value> result = init->call();

if (!result) {
    switch (result.error().kind()) {
    case rtype::luau::ErrorKind::kRuntimeError:
    case rtype::luau::ErrorKind::kTimeout:
    case rtype::luau::ErrorKind::kOutOfMemory:
    case rtype::luau::ErrorKind::kStackOverflow:
    case rtype::luau::ErrorKind::kUnknown:
        // result.error().message() / .traceback() for logging.
        break;
    default:
        break;
    }
    return;
}
```

## Values & Tables

`rtype::luau::Value` is the dynamic type used wherever the shape of the data
isn't known at the C++/Luau boundary ahead of time: variadic arguments,
table contents, and return values fetched without a template argument.

```c++
class Table;
class Function;
class Thread;

using Value = std::variant<
    std::monostate, // nil
    bool,
    double,
    std::string,
    Table,
    Function,
    Thread
>;
```

`Table` is a lightweight handle onto a Luau table (mirroring how the same
table is a reference type in Luau itself — copying a `Table` copies the
reference, not the contents):

```c++
class Table {
public:
    Value get(std::string_view key) const;
    void set(std::string_view key, Value value);

    Value get(int index) const; // 1-based, matching Luau's array convention
    void set(int index, Value value);

    std::size_t size() const;   // the '#' operator (array part length)

    // Iterates key/value pairs, like Luau's pairs().
    auto begin() const;
    auto end() const;
};
```

```c++
rtype::luau::Value hp = enemyTable.get("health");

if (double* health = std::get_if<double>(&hp)) {
    // ...
}
```

A `Runtime`'s own global scope is itself a `Table` — Luau's globals are
literally a table (`_G`), and the wrapper mirrors that instead of treating
"global" as a separate concept from "namespaced". `Runtime::globals()`
returns it, and it's the *only* place `.function(...)`, `.table(...)`,
`.constant(...)`, `.usertype<T>(...)`, `.handle<T>(...)`, and
`.enumeration<T>(...)` live — `Runtime` has no forwarding sugar for them, so
every top-level registration goes through `rt.globals()` explicitly:

```c++
rt.globals().function("cppHello").bind(/* ... */);
```

The practical upshot: there is only one vocabulary for "declare a name" —
`.function(...)`, `.table(...)`, `.constant(...)`, `.usertype<T>(...)`,
`.handle<T>(...)`, `.enumeration<T>(...)` — and it works identically
whether you call it on `rt.globals()`, on a nested `Table` like `engine` in
[Namespacing, Constants & Enums](#namespacing-constants--enums), or on an
[`Environment`](#sandboxing)'s own globals.

This is the *same* `Table` that appears inside `Value` above — there is no
separate "definition table" type. A `Table` fetched at runtime out of a
`Value` (e.g. arbitrary data a script built itself) supports the exact same
`.function(...)`/`.usertype<T>(...)`/etc. calls as `rt.globals()`, because a
Luau table doesn't distinguish "a namespace set up at startup" from "a plain
data table" — they're both just tables. The only guardrail is
[`seal()`](#sandboxing): it applies globally, so registering onto *any*
`Table` after sealing asserts, regardless of which one it is.

## Call Context

`CallContext` is the argument/return interface used by `.handler(...)`
bindings (the "advanced" form shown in [Basics](#basics)):

```c++
class CallContext {
public:
    // Raises a script-catchable kTypeMismatch error if the argument is
    // missing or isn't convertible to T.
    template <typename T> T get(std::string_view name) const;
    template <typename T> T get(std::size_t index) const;

    // Same as get(), but returns std::nullopt instead of raising, for
    // optional arguments.
    template <typename T> std::optional<T> tryGet(std::string_view name) const;

    std::size_t argCount() const;
    std::span<const Value> args() const; // every positional argument as a Value

    template <typename T> void ret(T value);
    void ret(); // no return value

    // Raises a Luau runtime error (surfaced to the caller as
    // ErrorKind::kRuntimeError) from within a C++ binding. Never throws a
    // C++ exception across the Luau boundary.
    [[noreturn]] void error(std::string message);

    // The coroutine currently executing this call. See Coroutines below.
    Thread& thread();

    // Suspends `thread()`. Must be the last statement the handler executes:
    // nothing below it runs. When the thread is later resumed, the resume
    // arguments become this call's return value(s) as seen by the script.
    [[noreturn]] void yield();
};
```

## Configuration

Controls which Luau libraries are available and enforces the memory/time
budgets a script runs under.

```c++
rtype::luau::RuntimeConfig cfg;

// os/io/debug are never exposed (see Non-Goals) — only gameplay-relevant
// libraries are opt-in.
cfg.libs = rtype::luau::Library::kBase
         | rtype::luau::Library::kMath
         | rtype::luau::Library::kTable
         | rtype::luau::Library::kString
         | rtype::luau::Library::kCoroutine;
cfg.memoryLimit = 64 * 1024 * 1024; // bytes; further allocations fail with kOutOfMemory
cfg.callBudget = 5ms;               // wall-clock budget per top-level call; see kTimeout
cfg.optimizationLevel = 1;          // Luau compiler -O level (0-2)

rtype::luau::Runtime rt{cfg};

// Override the default print function.
rt.globals().function("print")
    .variadic<rtype::luau::Value>("values")
    .ret<void>()
    .handler([](rtype::luau::CallContext& ctx) {
        for (const rtype::luau::Value& value : ctx.args()) {
            // C++ implementation of print.
        }
    });
```

## Generate type information

Emits a `.d.luau` type declaration for everything registered on a `Runtime`,
so editors/language servers can autocomplete the C++ API from Luau.

```c++
rtype::luau::Runtime rt;

rt.globals().function("cppHello")
    .bind([](std::string name) {
        return std::format("Hello {}, from C++!", name);
    })
    .names("name");

// Generates a Luau type declaration file for everything registered so far
// (functions, usertypes, handles, tables) — feed this to a Luau language
// server / editor for autocomplete over the C++ API.
rtype::luau::Result<std::string> result = rt.generateTypes();

std::cout << *result << std::endl;
```

Result:
```terminaloutput
declare function cppHello(name: string): string
```

## Sandboxing

Locking down a `Runtime` after setup, and isolating groups of scripts from
each other with separate global environments.

```c++
rtype::luau::Runtime rt;

rt.globals().function("hi")
    .bind([](std::string name) {
        return std::format("Hi {}!", name);
    });

// Seals the runtime to prevent further modifications. Useful for
// sandboxing: registering a function/usertype/handle/table after seal()
// is a programmer error and asserts (see Error Handling) rather than
// returning a Result, since it means C++ code — not a script — is
// misbehaving.
rt.seal();

// Loads the first sandboxed script.
auto a = rt.load("player.luau", srcA);

// Creates a new, separate global environment.
rtype::luau::Environment world = rt.createEnvironment("world");

// Both scripts share the same environment, and so see the same globals.
auto b = rt.load("house.luau", srcB, {.env = world});
auto c = rt.load("car.luau",   srcC, {.env = world});
```

`Environment::globals()` returns a `Table` too — the same type described in
[Values & Tables](#values--tables) — and it's what a script's bare global
assignments (`x = 1`, without `local`) actually write into, and what
`{.env = world}` binds a loaded script's top-level scope to. It is a
**C++-only accessor**: a script cannot say `world.globals()`, because inside
Luau `world` is just whichever plain `Table` was registered under that name
(the enemy-spawning namespace from [Handles](#handles)), not the C++
`Environment` object — it has no `.globals()` method to call.

## Usertypes

A usertype is a value type defined in C++ and exposed to Luau as userdata:
Luau's garbage collector owns the copy it holds, and it is collected like
any other Luau value once unreachable. This is different from a
[Handle](#handles), which references an object that C++ continues to own.

```c++
struct Color {
    float r, g, b, a;

    Color lerp(const Color& other, float t) const;
};

rt.globals().usertype<Color>("Color")
    .ctor<float, float, float, float>()
    .prop("r", &Color::r)
    .prop("g", &Color::g)
    .prop("b", &Color::b)
    .prop("a", &Color::a)
    .method("lerp", &Color::lerp)
    .meta("__tostring", [](const Color& c) {
        return std::format("Color({}, {}, {}, {})", c.r, c.g, c.b, c.a);
    })
    .meta("__eq", [](const Color& a, const Color& b) {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    })
    .meta("__add", [](const Color& a, const Color& b) {
        return Color{a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a};
    });
```
```luau
-- Creates a new usertype.
local color: Color = Color.new(1, 0, 0, 1)
local blended: Color = color:lerp(Color.new(0, 0, 1, 1), 0.5)
```

## Handles

A handle references an object that lives in the C++-owned game world. It
cannot be created from Luau — only returned by a C++-registered function —
and it never owns the object it points to.

```c++
rt.globals().handle<Enemy>("Enemy", [&world](rtype::luau::HandleId id) -> Enemy* {
        return world.find(id); // nullptr if destroyed
    })
    .method("takeDamage", &Enemy::takeDamage)
    .method("isAlive",    &Enemy::isAlive)
    .prop("health",       &Enemy::health, &Enemy::setHealth) // getter + setter
    .prop("name",         &Enemy::name);

rt.globals().table("world").function("createEnemy").bind([&world]() -> Enemy* {
    return world.spawn<Enemy>();
});
```
```luau
-- Creates a new enemy from a C++-provided function. The Luau runtime holds
-- an id that C++ resolves back to the Enemy on every access — the script
-- only ever sees the Enemy handle, never a raw pointer.
local enemy: Enemy = world.createEnemy()

print(enemy.name)

-- Two handles referring to the same id compare equal automatically.
assert(enemy == world.findEnemyByName(enemy.name))

-- ... elsewhere, C++ destroys the underlying Enemy ...

enemy:takeDamage(10) -- runtime error: "Enemy no longer exists" (kRuntimeError)
```

## Namespacing, Constants & Enums

Grouping related functions under nested tables instead of polluting Luau's
global scope, and exposing read-only constants/C++ enums alongside them.

```c++
// A bare global constant is declared exactly like a namespaced one — it's
// the same .constant(...) call, just on rt.globals() instead of a nested
// table (see Values & Tables).
rt.globals().constant("MAX_PLAYERS", 4);

rtype::luau::Table engine = rt.globals().table("engine");
rtype::luau::Table gas = engine.table("globalAudioSource");

gas.function("play").bind([](std::string_view name) { /* ... */ });
gas.function("volume").bind([](std::string_view name) { return /* ... */; });

// Read-only constant, namespaced under `engine` this time.
engine.constant("VERSION", "0.1.0");

// A C++ enum exposed as a table of named constants.
enum class Difficulty { kEasy, kNormal, kHard };

rt.globals().enumeration<Difficulty>("Difficulty")
    .value("Easy",   Difficulty::kEasy)
    .value("Normal", Difficulty::kNormal)
    .value("Hard",   Difficulty::kHard);
```
```luau
print(MAX_PLAYERS)      --> 4

engine.globalAudioSource.play("my_sound")
print(engine.VERSION)   --> "0.1.0"
print(Difficulty.Hard)  --> Difficulty.Hard, comparable with ==
```

## Coroutines

Gameplay scripts frequently need to describe a sequence of timed actions
(move, wait, shoot, wait, ...) directly, instead of hand-writing a state
machine. `Thread` wraps a Luau coroutine so the engine can drive that
sequence one resume at a time, once per frame (or once per scheduled wake-up).

```c++
class Thread {
public:
    enum class Status { kSuspended, kRunning, kDead, kErrored };

    Status status() const;

    // Resumes (or starts) the coroutine. `args` are the initial call
    // arguments on the first resume, or the yielded call's return values
    // on every resume after that (see CallContext::yield()).
    template <typename... Args>
    rtype::luau::Result<rtype::luau::Value> resume(Args&&... args);
};
```

A yieldable C++ binding, e.g. a `wait()` builtin that hands the thread to a
scheduler instead of blocking:

```c++
rt.globals().function("wait")
    .arg<double>("seconds")
    .handler([&scheduler](rtype::luau::CallContext& ctx) {
        double seconds = ctx.get<double>("seconds");

        scheduler.wakeAfter(ctx.thread(), seconds);
        ctx.yield();
    });
```

```luau
local function patrol()
    moveTo(10, 0)
    wait(1.0)
    moveTo(-10, 0)
    wait(1.0)
end
```

```c++
// Creating and driving a Thread from C++, e.g. one per spawned enemy:
rtype::luau::Thread patrol = script->thread("patrol");

// In the scheduler, once a thread's wake-up time has elapsed:
rtype::luau::Result<rtype::luau::Value> r = patrol.resume();

if (!r && patrol.status() == rtype::luau::Thread::Status::kErrored) {
    // Log r.error() and drop the thread.
}
```

## Hot Reloading

Recompiling and swapping a script's implementation while the engine keeps
running, for fast content iteration.

```c++
rtype::luau::Result<void> reload = script->reload(newSrc);

if (!reload) {
    // Compile error in the new source (kSyntaxError) — the OLD script
    // keeps running unmodified.
}
```

Reloading recompiles and re-runs the script's top-level chunk, so any state
kept only in Luau locals at chunk scope is reset. Game state that must
survive a reload (an enemy's current health, position, ...) should live on
the C++ side and be reached through a [Handle](#handles), not through Luau
variables. `Function`/`Thread` handles obtained before the reload are
invalidated — re-fetch them via `script->function(...)` / `script->thread(...)`
afterwards.

```luau
-- bad_wave.luau — BAD: authoritative progress lives in a top-level local.
local enemiesSpawned = 0

function update(dt)
    enemiesSpawned += 1
    world.createEnemy()
end
```

Reloading `bad_wave.luau` — even just to tweak an unrelated line — re-runs
the chunk from scratch and resets `enemiesSpawned` to `0`. Any logic built
on it (e.g. "spawn a boss every 10th enemy") silently restarts from zero
mid-level, even though dozens of enemies already spawned before the reload.

```luau
-- good_wave.luau — GOOD: authoritative progress lives on the C++ side,
-- reached through a C++-registered function, which reload() never touches —
-- only the chunk's own locals are reset.
function update(dt)
    world.createEnemy()

    if world.enemiesSpawned() % 10 == 0 then
        world.createBoss()
    end
end
```

Here the spawn count survives the reload, because it lives in the C++ game
world rather than in a Luau local. How the engine exposes persistent
script state (e.g. a per-level key/value store) is up to the engine
implementation and is not part of this generic library.

## Multithreading

A `Runtime` owns the *shared* state: C++ registrations (functions,
usertypes, handles, tables) and the compiled bytecode cache for every
loaded script. A single Luau VM cannot be entered from two threads at once,
so each thread that wants to run scripts in parallel (e.g. one ECS job per
worker thread) pulls its own lightweight VM out of the shared `Runtime`:

```c++
rtype::luau::WorkerVm vm = rt.forThread();

// vm behaves like a Runtime for loading/calling scripts, but its Luau
// state — globals, GC heap, memory/time budget — is private to the
// calling thread.
rtype::luau::Result<rtype::luau::Script> script = vm.load("enemy_ai.luau", src);
```

A `WorkerVm` (and any `Script`/`Function`/`Thread` it produces) must only
ever be used from the thread that created it. Registrations made on the
`Runtime` before any `forThread()` call are visible to every `WorkerVm`;
`Runtime::seal()` still applies globally. Handle-resolution callbacks (e.g.
`world.find(id)` in the [Handles](#handles) example) may be invoked
concurrently from multiple worker threads — making those callbacks
thread-safe is the responsibility of the integrating C++ code, not this
wrapper.

## Non-Goals

To hit the project deadline, the following are intentionally out of scope
for the first version:

- **`os`/`io`/`debug` libraries.** Never exposed to scripts — no filesystem
  or process access from Luau, and no debug-library introspection.
- **Cross-file module resolution (`require`).** Every script is compiled
  explicitly via `Runtime::load`/`WorkerVm::load`; there is no implicit
  module search path.
- **A debugger/breakpoint UI.** `Error::traceback()` is available for
  logging, but there is no interactive debugging support.
- **General-purpose `buffer`/native `vector` bindings.** Only what specific
  gameplay features need will be wrapped, on demand, rather than the full
  Luau standard library surface.
- **Pre-emptive script interruption.** `callBudget` (see
  [Error Handling](#error-handling)) is cooperative, not a hard real-time
  guarantee.
