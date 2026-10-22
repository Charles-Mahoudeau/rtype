# Luau module error handling

How the `rtype::luau` module reports failures. The goal is one rule, so a caller can tell from a
signature whether a call can fail at run time and how.

## The rule

A failure is reported in one of two ways, depending on who caused it.

| Cause | Examples | Reported as |
| --- | --- | --- |
| The script, the data or the environment | compile or runtime error in a script, a value of the wrong type, a value that belongs to another Lua state, the Lua stack that cannot grow, out of memory, a timeout | `Result<T>` holding an `Error` |
| A bug in the calling C++ code (API misuse) | a null `lua_State*`, a moved-from handle, an operation the object does not support | an exception derived from `exceptions::Base` |

In short: **if a correct program can still hit it, it is a `Result`; if only a wrong program can hit it, it
throws.**

### `Result<T>` for recoverable failures

- The function returns `Result<T>` (`Result<void>` when there is no value) and is marked `[[nodiscard]]`.
- It never throws for these failures. Build the error with `Failure{ErrorKind::..., message}`; use
  `forwardError<T>()` to pass on the error of a sub-operation.
- Pick the `ErrorKind` that matches the cause (`kRuntime`, `kSyntax`, `kTypeMismatch`, `kStackOverflow`,
  `kOutOfMemory`...). A stack that cannot grow is `kStackOverflow`, not an exception.
- Document the failures in `@return` ("or a Failure of kind `kTypeMismatch` if...").

### Exceptions for API misuse

- The exceptions live in `Exceptions.hpp` and all derive from `exceptions::Base`:
  `InvalidState` (null state), `InvalidRef` (empty or moved-from handle), `InvalidOperation` (an operation the
  object does not support).
- They signal a programmer error. Callers are not expected to catch them, and a program that never misuses the
  API never sees one.
- Document them with `@throws`. A `@throws` of a recoverable failure (such as a stack overflow) is a sign the
  function should return a `Result` instead.

## Constructors

A constructor cannot return a `Result`, so a type whose creation can fail has a **static factory** returning
`Result<T>` (`Runtime::create`, `Table::create`, `Table::fromStack`). The real constructor is private and
`noexcept` when it can be: the factory does everything that can fail first, then hands the finished parts to it.
No instance of the type is ever half built.

When a private constructor has to run code that can throw, the factory catches at its boundary and turns the
exception into a `Failure` (see `Runtime::create`).

A plain value type whose construction cannot fail (`Value`, `ValuePack`) keeps ordinary public constructors.

## The native function boundary

A native function bound to Luau (`Function`) is the one place where C++ code is called from a script. It must
not let a C++ exception unwind through Luau, so:

- a handler reports a failure by throwing an `exceptions::Base`, which becomes a Lua error with its message;
- anything else a handler throws is turned into a generic Lua error;
- a `Failure` met while reading the arguments or pushing the results becomes a Lua error too.

In the other direction, `Function::call` runs the script in protected mode and returns the script's error as a
`Failure` of kind `kRuntime`.

## Adding a function

1. Can a correct program see this failure (script, data, environment)? Return `Result<T>`.
2. Is it a mistake in the calling code? Throw from `exceptions/`, document it with `@throws`, and check it before
   touching the Lua stack, so the stack is left as it was.
3. Either way, leave the Lua stack as it was found when the call fails.
