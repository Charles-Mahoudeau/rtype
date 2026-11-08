# JSON

The `rtype.Json` plugin (`src/rtype/plugins/json/`, library `plugin-json`) gives scripts a JSON codec: encode a table,
decode a text (an HTTP body, an AI model's reply, a save). It requires no other plugin and runs on every side.

## Why it looks like this

- **Luau, not engine calls.** An engine call carries reflected structs ([Luau bridge](ecs/features/19-luau-bridge.md)),
  never a table of any shape, so the codec cannot be a call. The plugin ships `JsonForms.luau`, embedded in the library
  like every `<Name>Forms.luau`, which defines `engine.json`. It is the same mechanism as `engine.http.request`.
- **Deterministic.** The same value always gives the same text: object keys are sorted, numbers have one spelling
  (whole numbers under 2^53 without a fraction, others with `%.17g`, which reads back exactly). A prompt, a hash or a
  test can rely on it. `engine.json.map` is the way out when a layout must keep a chosen order.
- **Mistakes are errors, untrusted input is data.** `encode` and `decode` raise a Luau error (a message starting
  `json:`); `tryDecode` never raises, for text a script does not control.
- **Bounded.** Nesting is limited to 64 levels, both ways: a hostile body cannot exhaust the stack, and a table that
  contains itself is an error, not an endless loop.

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
| `engine.json.decode(text)` | The value. Objects and arrays are tables, `null` is `nil`. Errors, naming the byte (`json: expected ':' at byte 6`): malformed text, leading zeros, a number out of range, trailing data, nesting over 64, a `text` that is not a string |
| `engine.json.tryDecode(text)` | `value` on success, or `nil, message` on error. A text that is `null` gives `nil` and no message |
| `engine.json.map(entries)` | `entries` (a list of `{ key, value }`) marked, so that it encodes as an object with the keys in that order; returns `entries` |

Choices a caller should know: `\u` surrogate pairs decode to one character, a lone surrogate to U+FFFD; a `null`
inside an array takes no place (the elements after it move down) and a `null` field is absent; the empty object
decodes to `{}`, which encodes back as `[]` (use `map({})` for `{}`); control characters are escaped on output;
strings are not checked for UTF-8.

## Tests

`tests/engine/ScriptJsonTests.cpp` runs scripts on an `App` with the plugin: encoding, the ordered map, round trips,
every error, the depth limit. `tests/plugins/JsonPluginTests.cpp` checks the plugin's name and that it requires nothing.

## Later

- A native (C++) codec if profiling shows a script's cost on large bodies.
