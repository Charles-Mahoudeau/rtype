# HTTP

The HTTP plugin lets your scripts call a web API: an AI model, a leaderboard, your own backend. A request never
blocks the game: you send it, keep playing, and its response arrives a few frames later as an event.

Add it on the **server**, and name the hosts your game talks to:

```luau
-- project.luau
if not engine.isClient then
	engine.settings.plugins = { "rtype.Http" }
	engine.settings.http.allowedHosts = { "api.example.com" }
	engine.settings.http.secrets = { "EXAMPLE_API_KEY" }
end
```

Put it on the server only: the server owns the gameplay, and the API keys you use stay on the machine that runs it.
Nothing can be reached until you list its host in `allowedHosts`.

## A first request

```luau
-- scripts/server/ask.luau
if engine.isClient then
	return
end

local pending = nil

engine.ecs.system("game.Ask", {
	phase = "Update",
	query = {},
	calls = { "rtype.http.request" },
	events = { read = { "rtype.HttpResponse" } },
	run = function(ctx)
		if pending == nil then
			pending = engine.http.request(ctx, {
				method = "POST",
				url = "https://api.example.com/v1/answer",
				headers = {
					Authorization = "Bearer {{env:EXAMPLE_API_KEY}}",
					["Content-Type"] = "application/json",
				},
				body = '{"question":"where should the miners go?"}',
			})
		end
		for response in ctx.events.read("rtype.HttpResponse") do
			if response.request == pending then
				if response.ok then
					print("answer:", response.body)
				else
					print("failed:", response.status, response.error)
				end
				pending = nil
			end
		end
	end,
})
```

- The system lists `rtype.http.request` under `calls` and `rtype.HttpResponse` under `events.read`, as every system
  must declare what it uses.
- `engine.http.request` returns the request's id at once. Keep it to recognise its response.
- The response comes as an `rtype.HttpResponse` event, in a later frame. A system of the `Update` phase reads it in
  the frame the request ends.

## `engine.http.request(ctx, request)`

`ctx` is the system's context; give `nil` to send a request while the script loads (at the top level of a file).

| Key | Type | Default | Meaning |
|---|---|---|---|
| `url` | string | **required** | `https://host/path`; the host must be in `http.allowedHosts` |
| `method` | string | `"GET"` | `GET`, `POST`, `PUT`, `PATCH`, `DELETE` or `HEAD`, any case |
| `headers` | table | none | Header name to value, both strings: `{ ["Content-Type"] = "application/json" }` |
| `body` | string | `""` | Sent as is, of any length (a JSON text, for instance). A `GET` or `HEAD` has none |
| `timeout` | number | `http.timeout` | Seconds; it can be shorter than the setting, never longer |

It is a thin form over the engine call `rtype.http.request`, which takes the same fields with `headers` as text, one
`Name: value` per line. `rtype.http.cancel` (`{ request = id }`) stops a request; it returns `true`, or `false` when
it had already ended. A cancelled request sends no event.

## `rtype.HttpResponse`

Sent once per request, whatever happened, except for a cancelled one.

| Field | Type | Meaning |
|---|---|---|
| `request` | number | The id `engine.http.request` returned |
| `status` | number | The HTTP status (`200`, `404`...); `0` when no response came |
| `ok` | boolean | A response came with a status from 200 to 299 |
| `body` | string | The response's body, of any length; empty when no response came |
| `error` | string | Why no response came: no network, the timeout, a body over `http.maxResponseBytes`; empty otherwise |

An error status is still a response: a `404` has `status = 404`, `ok = false` and an empty `error`.

## Secrets

Never write an API key in a script. Write `{{env:NAME}}` in a header instead, list `NAME` in `http.secrets`, and set
the environment variable `NAME` on the machine that runs the server:

```sh
EXAMPLE_API_KEY=sk-... ./r-type_server
```

The engine puts the value in the header when it sends the request. Your scripts never see it, and only the variables
`http.secrets` lists can be named.

## Settings

The `http` section of `project.luau`; every key is optional.

| Key | Default | Meaning |
|---|---|---|
| `allowedHosts` | `{}` | The hosts a request may go to, without scheme, port or path: `{ "api.example.com" }` |
| `allowHttp` | `false` | Also allow `http://` URLs, for a server on your own machine |
| `maxConcurrent` | `8` | Requests in flight at once; the next ones wait their turn, in order |
| `timeout` | `30` | The longest a request may take, in seconds |
| `maxResponseBytes` | `1048576` | A longer response ends with an error instead |
| `secrets` | `{}` | The environment variables a header may name as `{{env:NAME}}` |

Redirects are not followed: a request gets the redirect itself as its response (`status` 301, 302...).

## Errors

A request that cannot be sent is a Luau error in the system that sends it, naming the cause:

| Message | Cause |
|---|---|
| `engine.http.request: expected a 'url'` | No `url` |
| `engine.http.request: unexpected 'link', expected ...` | A key the form does not know |
| `engine.http.request: header 'A': must be a string` | A header value that is not a string |
| `... the host 'elsewhere.com' is not in http.allowedHosts` | A host you did not list |
| `... only https:// URLs are allowed` | An `http://` URL without `http.allowHttp` |
| `... Unknown HTTP method 'BREW'` | A method that is not one of the six |
| `... The secret 'NAME' is not in http.secrets` | A `{{env:NAME}}` you did not list |
| `... The secret 'NAME' is not set in the environment` | A listed secret whose variable is not set |

A request that was sent and failed is not an error: it ends with an `rtype.HttpResponse` whose `status` is `0` and
whose `error` says why.

!!! warning "Server only"
    `rtype.http.request` and `rtype.HttpResponse` exist where the plugin is added. With it on the server only, give
    the systems that use them `side = "server"` (the default), in a file that starts with
    `if engine.isClient then return end`.

## See also

- [Systems](systems.md): `calls`, `events` and sides.
- [Project settings](project-settings.md): adding plugins per side.
- [Engine calls](engine-calls.md): how calls take their arguments.
