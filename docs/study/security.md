# Security

What can go wrong with each technology we use and with our own code, what we do about it, and how we keep watching
over time.

## Threat model

| Who | Can | Must not be able to |
|---|---|---|
| A remote attacker on the network | Send any bytes to the server's UDP port, spoof source addresses | Crash or stall the server, exhaust its memory, impersonate a player, use the server to flood someone |
| A cheating player | Run a modified client, send any input or packet | Change the world directly, teleport, shoot from anywhere, edit another player |
| A malicious game or plugin author | Ship scripts, assets and native plugins | Run native code on a release player's machine without a signature the project vouches for |
| A careless game script | Loop, error, read files | Take the game down, escape the game folder, read secrets |

## By technology

| Technology | Main risks | What we do |
|---|---|---|
| **Our UDP protocol** | Malformed packets, buffer overflows, amplification, spoofing, resource exhaustion | Bounds-checked reader that invalidates itself instead of throwing; every count checked against the bytes left before allocating; caps on clients, pending challenges, reliable windows, queues, strings; challenge + random session token; padded handshake requests; fuzz-style tests on malformed bytes ([protocol §12](../network/protocol.md#12-security-considerations)) |
| **The game simulation** | Cheating clients | The server is authoritative: clients send held inputs only, never positions; shooting is an input; custom packets to the server are declared, typed and validated |
| **Asio** | Bugs in a socket wrapper | We use only UDP sockets; on Windows we turn off `SIO_UDP_CONNRESET` so a crashed client cannot stop the receive loop |
| **Luau** | Scripts escaping their sandbox, infinite loops, errors | `luaL_sandbox`, frozen standard libraries and `engine` table, one environment per file; `require` cannot leave `scripts/`; a script error is logged and the system skipped, never a crash ([`luau/sandbox.md`](../luau/sandbox.md)) |
| **Native plugins (shared libraries)** | Arbitrary code execution | A release build loads a game plugin only with an Ed25519 signature chained to the project's root key (libsodium); nothing in `project.luau` can turn the check off ([`Plugins.md`](../Plugins.md#signatures)) |
| **libcurl + OpenSSL** (HTTP plugin) | Requests to unexpected hosts, leaked API keys, TLS downgrade | `allowedHosts` empty by default, `https://` only, no redirects, a response size limit; secrets come from environment variables listed in `http.secrets` and never pass through Luau; OpenSSL is built from source and statically, not the system's ([`Http.md`](../Http.md)) |
| **Assets and scene files** | Malformed images, fonts, capsules; path traversal | Paths are resolved inside `<game>/assets` only (`isInsideDirectory`); readers report errors with a position and never crash |
| **Saves** | Corrupt or edited files | Atomic writes, checksum, checked names ([Storage](storage.md)); anything that must not be cheated is kept on the server |
| **Admin channel** (editor ↔ server) | Remote control of a server | Debug runs only, bound to the loopback interface, every request carries a secret token |
| **Vulkan, GLFW, miniaudio, Jolt, Yoga, stb_image** | Bugs in native libraries fed with game data | Only the game's own assets reach them; versions pinned in `xmake.lua` and updated on purpose |

## Known limits

- **No encryption.** Traffic, including tokens, can be read by anyone on the path. The protocol prevents impersonation
  from elsewhere, not eavesdropping. Adding encryption would mean a key exchange in the handshake (libsodium has it).
- **Signatures cannot be revoked** yet: a leaked developer key stays valid until the root key changes.
- **Local saves can be edited** by their player: the checksum detects damage, not tampering.

## Watching over time

| What | How |
|---|---|
| Our own code | Every pull request is reviewed and runs CI on Linux, macOS and Windows: build, all tests (including malformed-bytes tests of the network and every decoder), clang-format and clang-tidy (with its `bugprone`, `clang-analyzer` and `cppcoreguidelines` checks) on the changed files |
| Network robustness | Tests feed truncated, oversized and random bytes to every decoder; the link simulator loses, duplicates and reorders datagrams |
| Dependencies | Versions are pinned by range in `xmake.lua` and come from `xmake-repo`, which tracks upstream releases; a version bump is a pull request that runs the whole CI. Next step: watch security advisories of our dependencies (GitHub advisories for OpenSSL, libcurl, libsodium; subscribing to their release announcements) and record each check in the pull request that bumps them |
| Releases | Each release zip has a `SHA256SUMS` entry and a GitHub provenance attestation; macOS binaries are ad-hoc signed, Windows signing is pending ([`release/signing.md`](../release/signing.md)) |
| Reporting | Security issues are reported through the repository's issues, labeled for the area they touch |
