# JSON

The `rtype.Json` plugin (`src/rtype/plugins/json/`, library `plugin-json`) gives scripts a JSON codec: encode a table,
decode a text (an HTTP body, an AI model's reply, a save). It requires no other plugin and runs on every side.

## Why it looks like this

- **nlohmann::json in C++, behind real engine calls.** `rtype.json.encode` and `rtype.json.decode` ([Luau
  bridge](ecs/features/19-luau-bridge.md)) do the actual work: number formatting, string escaping and JSON grammar are
  nlohmann::json's, not a hand-written Luau parser's. An engine call carries reflected struct fields, never a table of
  arbitrary shape, so the value itself cannot be one field: `EncodeArguments` and `DecodeResult` (`JsonCalls.hpp`)
  carry it flattened into a pre-order node stream instead (a kind per node, plus parallel lists of the numbers,
  child counts and string bytes the nodes need, read back recursively). `JsonForms.luau` is a thin layer over the
  calls: on the way in, it only tells a plain Luau table from an array or an `engine.json.map()` ordered object
  (table-shape inspection, not JSON logic); on the way out, it only rebuilds a Luau value from the stream. It never
  formats a number or escapes a string itself.
- **Deterministic.** The same value always gives the same text: a plain object's keys are written sorted (in C++, so
  `JsonForms.luau` does not need to sort them itself); numbers are nlohmann::json's own shortest round-tripping text,
  except that a whole number under 2^53 is written without a fraction. A prompt, a hash or a test can rely on it.
  `engine.json.map` is the way out when a layout must keep a chosen order.
- **Mistakes are errors, untrusted input is data.** `encode` and `decode` raise a Luau error (a message starting
  `json:`); `tryDecode` never raises, for text a script does not control.
- **Bounded.** Nesting is limited to 64 levels, both ways, checked in C++ even when a script calls `rtype.json.encode`
  or `rtype.json.decode` directly, skipping `JsonForms.luau`: a hostile body cannot exhaust the stack, and a table
  that contains itself is an error, not an endless loop.
- **A Luau string is bytes, not checked UTF-8.** `encode` writes an invalid byte sequence as is, instead of
  nlohmann::json's default of raising on it. `decode`, reading a text a script did not write, does validate UTF-8 (one
  more way malformed input is rejected): an invalid sequence in the JSON text is a decode error.

## From scripts

```luau
-- project.luau
engine.settings.plugins = { "rtype.Json" }

local text = engine.json.encode({ name = "ship", pos = { x = 1, y = 2 }, tags = { "a", "b" } })
-- {"name":"ship","pos":{"x":1,"y":2},"tags":["a","b"]}
local value, err = engine.json.tryDecode(response.body)
```

| Helper | Result |
|---|---|
| `engine.json.encode(value)` | The text. `nil` is `null`; a table of `1..n` with no hole (and the empty table) is an array; a table with string keys is an object with **sorted** keys. Errors: NaN or infinite number, function, thread or userdata, a table mixing array and string keys (or with holes, or non-string keys), nesting over 64 |
| `engine.json.decode(text)` | The value. Objects and arrays are tables, `null` is `nil`. Errors, naming the byte of a malformed text (an invalid token, an invalid UTF-8 sequence, trailing data), a number no finite double can hold, nesting over 64, a `text` that is not a string |
| `engine.json.tryDecode(text)` | `value` on success, or `nil, message` on error. A text that is `null` gives `nil` and no message |
| `engine.json.map(entries)` | `entries` (a list of `{ key, value }`) marked, so that it encodes as an object with the keys in that order; returns `entries` |

| Call | Arguments | Result |
|---|---|---|
| `rtype.json.encode` | `kinds`, `numbers`, `counts`, `text`, `textLengths`: a value flattened into a pre-order node stream (`JsonCalls.hpp`) | `text`, the JSON text |
| `rtype.json.decode` | `text`, the JSON text to parse | `kinds`, `numbers`, `counts`, `strings`, `stringLengths`: the parsed value, flattened the same way |

`engine.json.encode` and `engine.json.decode` are thin forms over these calls, built with `engine.call` (no `ctx`
needed: the calls take no system resource, so a system never has to list them under `calls`). Scripts are not expected
to make the calls directly; `JsonForms.luau` is the stream's only reader and writer.

Choices a caller should know: `\u` surrogate pairs decode to one character, and a lone surrogate is a decode error; a `null`
inside an array takes no place (the elements after it move down) and a `null` field is absent; the empty object
decodes to `{}`, which encodes back as `[]` (use `map({})` for `{}`); control characters are escaped on output;
`encode` does not check a string for valid UTF-8 (it writes the bytes as given), `decode` does.

## Tests

`tests/engine/ScriptJsonTests.cpp` runs scripts on an `App` with the plugin: encoding, the ordered map, round trips,
every error, the depth limit. `tests/plugins/JsonPluginTests.cpp` checks the plugin's name and that it requires nothing. The script tests also call
`rtype.json.encode` directly, to check that the 64-level limit holds without `JsonForms.luau`.

## Later

- A streaming decode for a response too large to flatten into one result, if profiling ever shows it is needed.
