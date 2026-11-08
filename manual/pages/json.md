# JSON

The JSON plugin turns tables into JSON text and back: to build the body of an [HTTP](http.md) request, read a
response, or keep a save in a readable form. Add it on every side that needs it:

```luau
-- project.luau
engine.settings.plugins = { "rtype.Json" }
```

## Encode

```luau
local text = engine.json.encode({ name = "ship", hp = 3, tags = { "a", "b" } })
-- {"hp":3,"name":"ship","tags":["a","b"]}
```

- A table with string keys is an object. Its keys are **sorted**, so the same table always gives the same text.
- A table of `1..n` with no gap is an array. The empty table is `[]`.
- `nil` is `null`; strings are escaped; whole numbers have no fraction.
- It raises an error for NaN or infinite numbers, functions, a table that mixes array and string keys (or has gaps),
  and nesting deeper than 64 levels.

### Keeping a key order

Sorting is not always what you want, a prompt for instance. `engine.json.map` takes a list of `{ key, value }`:

```luau
engine.json.encode(engine.json.map({ { "role", "user" }, { "content", "hi" } }))
-- {"role":"user","content":"hi"}
```

Maps nest, and can sit inside ordinary tables and arrays.

## Decode

```luau
local reply = engine.json.decode('{"ok":true,"items":[1,2,3]}')
print(reply.items[2]) -- 2
```

Objects and arrays become tables, `null` becomes `nil`. A `null` in an array takes no place: `[1,null,2]` gives
`{ 1, 2 }`. Bad text raises an error that names the byte: `json: expected ':' at byte 6`.

### Text you do not trust

An AI reply or a network body can be anything. `tryDecode` never raises:

```luau
local value, err = engine.json.tryDecode(response.body)
if err ~= nil then
	print("bad reply: " .. err)
	return
end
```

It returns the value, or `nil` and the message. (A text that is just `null` gives `nil` and no message.)

## Limits

Nesting stops at 64 levels, so a hostile text cannot crash the game. The empty object decodes to `{}`, which encodes
back as `[]`; write `engine.json.map({})` to get `{}`.
