# HTTP

The `rtype.Http` plugin (`src/rtype/plugins/http/`, library `plugin-http`) lets a game's scripts send HTTPS requests
and read the responses: to call a web API, such as an AI model's, from gameplay. It requires no other plugin.

## Why it looks like this

- **Nothing waits.** A request takes from milliseconds to many seconds (an AI model's answer), and a frame lasts 16 ms.
  `rtype.http.request` returns an id at once; the response comes later as an `rtype.HttpResponse` event. There is no
  thread: a curl multi handle is advanced, without waiting, by one system per frame (`rtype.Http.Poll`, PreUpdate),
  so systems of Update read a response in the frame it ends.
- **The server is authoritative, and keeps the secrets.** A game lists the plugin on the side that makes the requests,
  the server in a networked game (`if not engine.isClient then ... end` in `project.luau`). An API key is never
  written in a script: a header names it as `{{env:NAME}}`, the plugin reads the environment variable when it sends
  the request, and only the variables `http.secrets` lists can be named. The key never passes through Luau.
- **A game reaches only what it names.** `http.allowedHosts` is empty by default, so nothing can be reached until the
  game lists its hosts. URLs are `https://` only, unless `http.allowHttp` (a local server); no redirect is followed,
  since it could lead to another host.
- **Failures are data, mistakes are errors.** A request that cannot be sent (a URL that is not one, a host that is not
  allowed, a header without `:`, an unknown secret, an unknown method, a body on a GET) throws an `HttpException`,
  which a script gets as a Luau error naming the cause. A request that was sent and failed (no network, a timeout, a
  response over `http.maxResponseBytes`) ends with an `HttpResponse` whose `status` is 0 and whose `error` says why.
  An error status (404, 500) is a response: `status` holds it, `ok` is false.
- **Long texts.** A request's URL, headers and body and a response's body are `std::string` fields: call data and
  events of any length ([Systems](ecs/features/12-systems.md#engine-calls)). A script reads an event that owns
  memory as a copy made with its copy hook ([Luau bridge](ecs/features/19-luau-bridge.md)).

## Parts

| File | What |
|---|---|
| `HttpPlugin` | Inserts the `HttpClient`, registers the event and the calls, adds `rtype.Http.Poll` |
| `HttpSettings` | The `http` section of `project.luau`, checked when the plugin is created |
| `HttpClient` | The resource `rtype.HttpClient`: `send()`, `cancel()`, `poll()`; at most `maxConcurrent` in flight, the next ones queued (highest `priority` first, send order among equals, at most `maxQueued`) |
| `detail/CurlClient` | The libcurl transfers. Private: curl reaches no public header |
| `HttpEvents.hpp` | `HttpResponse` |
| `HttpCalls.hpp` | `rtype.http.request`, `rtype.http.cancel`, and the `engine.http.request` helper (`HttpForms.luau`) |

libcurl is a private package of the plugin: Schannel on Windows, OpenSSL 3 on Linux and macOS (the xmake package
picks it). `curl_global_init()` is reference-counted by curl and called once per `HttpClient`.

## Settings

| Key | Default | Meaning |
|---|---|---|
| `allowedHosts` | `{}` | The hosts a request may go to, exactly, any case: `"api.example.com"` |
| `allowHttp` | `false` | Also allow `http://` (a local server, the tests) |
| `maxConcurrent` | `8` | Requests in flight at once; the next ones wait their turn |
| `maxQueued` | `256` | Requests waiting for a slot; one more ends at once with the error `queue_full` (0: no waiting) |
| `timeout` | `30` | Seconds a request may take **from its send, queue time included**; a request may ask for less, never more |
| `maxResponseBytes` | `1048576` | A longer response body ends the request with an error |
| `secrets` | `{}` | The environment variables a header may name as `{{env:NAME}}` |

## From scripts

```luau
-- project.luau (server)
engine.settings.plugins = { "rtype.Http" }
engine.settings.http.allowedHosts = { "api.example.com" }
engine.settings.http.secrets = { "EXAMPLE_API_KEY" }

-- a server system
engine.ecs.system("game.Ask", {
    phase = "Update",
    query = {},
    calls = { "rtype.http.request" },
    events = { read = { "rtype.HttpResponse" } },
    run = function(ctx)
        local id = engine.http.request(ctx, {
            method = "POST",
            url = "https://api.example.com/v1/answer",
            headers = { Authorization = "Bearer {{env:EXAMPLE_API_KEY}}", ["Content-Type"] = "application/json" },
            body = '{"question":"where to mine?"}',
        })
        for response in ctx.events.read("rtype.HttpResponse") do
            if response.ok then print(response.body) else print(response.status, response.error) end
        end
    end,
})
```

| Call | Arguments | Result |
|---|---|---|
| `rtype.http.request` | `method` (`"GET"`), `url`, `headers` (`Name: value` lines), `body`, `timeout` (0: the setting), `priority` (whole number, `0`; higher is sent first) | `request`, its id |
| `rtype.http.cancel` | `request` | `cancelled`: false when it already ended |

`engine.http.request(ctx, { url, method, headers, body, timeout, priority })` is the same call with `headers` as a table; `ctx` is
nil while the script loads. `rtype.HttpResponse` carries `request`, `status`, `ok`, `body` and `error`.

## Queue and deadline

A request over `maxConcurrent` waits in a queue ordered by `priority` (higher first; send order among equals). The queue
holds `maxQueued` requests: the next one is not dropped silently, it ends with an `HttpResponse` whose `error` is
`queue_full` (`kQueueFull`), in the next poll. The timeout is a deadline set at `send()`: a request that waits too long
is never sent and ends with the error `timeout`; one that starts gets only the time left. So an agent's wait is bounded
by `timeout`, whatever the queue.

## Tests

`tests/plugins/HttpTests.cpp` runs every case against `LocalHttpServer` (`tests/plugins/LocalHttpServer.hpp`), a plain
HTTP server on 127.0.0.1 built on Asio, so no test reaches the internet. `HttpClient.DISABLED_ReachesARealHttpsServer`
does (`--gtest_also_run_disabled_tests`), to check that a build speaks TLS. `tests/engine/ScriptHttpTests.cpp` sends
requests from scripts.

## Later

- JSON for scripts (encode a table, decode a body): today a script writes and reads the text itself.
- Streaming responses (an AI model's tokens as they come).
- Hot reload of the `http` settings.
