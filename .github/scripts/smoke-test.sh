#!/bin/sh
# Smoke test of a release zip: checks it the way a user would, by unzipping it into an empty directory and running it.
#
#   .github/scripts/smoke-test.sh <aylo-engine-...zip>
#
# Checks the layout, the hashes of the manifest, the version the runtime reports, the dependencies of runtime/ (with
# check-self-contained.sh), a server started on templates/game with a headless client connecting to it, and aylo-sign.
# Run it on a machine with no toolchain (a bare container) for the answer that counts. Exits 1 if a check fails.

if [ $# -ne 1 ] || [ ! -f "$1" ]; then
    echo "usage: $0 <release zip>" >&2
    exit 2
fi

here=$(cd "$(dirname "$0")" && pwd)
zip="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
work=$(mktemp -d)
server_pid=""
client_pid=""
failures=0

cleanup() {
    [ -n "$client_pid" ] && kill "$client_pid" 2>/dev/null
    [ -n "$server_pid" ] && kill "$server_pid" 2>/dev/null
    rm -rf "$work"
}
trap cleanup EXIT INT TERM

ok() { echo "ok:   $*"; }
warn() { echo "warn: $*"; }
fail() {
    echo "FAIL: $*" >&2
    failures=$((failures + 1))
}

# --- Unzip into an empty directory ------------------------------------------------------------------------------------
mkdir "$work/unzipped" || exit 2
if ! unzip -q "$zip" -d "$work/unzipped"; then
    echo "FAIL: $zip does not unzip" >&2
    exit 1
fi
count=$(find "$work/unzipped" -mindepth 1 -maxdepth 1 | wc -l | tr -d ' ')
root=$(find "$work/unzipped" -mindepth 1 -maxdepth 1 -type d | head -n 1)
if [ "$count" -ne 1 ] || [ -z "$root" ]; then
    echo "FAIL: the zip must hold exactly one folder, it holds $count entries" >&2
    exit 1
fi
ok "unzipped to one folder: $(basename "$root")"

# --- Layout -----------------------------------------------------------------------------------------------------------
runtime="$root/runtime/aylo-runtime"
for file in manifest.json README.md VERSION templates/game/project.luau runtime/licenses/THIRD_PARTY_NOTICES.md; do
    if [ -f "$root/$file" ]; then ok "has $file"; else fail "missing $file"; fi
done
for file in runtime/aylo-runtime tools/aylo-sign; do
    if [ -x "$root/$file" ]; then ok "$file is executable"; else fail "$file is missing or not executable"; fi
done
for folder in runtime/plugins runtime/plugins/backends templates/game/scripts templates/game/assets; do
    if [ -d "$root/$folder" ]; then ok "has $folder/"; else fail "missing $folder/"; fi
done
# plugins/ links to the libraries next to the executable on Linux and macOS (a second copy would load twice): none may dangle.
dangling=$(find -L "$root" -type l | wc -l | tr -d ' ')
if [ "$dangling" -eq 0 ]; then ok "no dangling symbolic link"; else fail "$dangling dangling symbolic links"; fi
if [ -x "$runtime" ]; then
    # --- Version ------------------------------------------------------------------------------------------------------
    reported=$("$runtime" --version | sed -n 1p)
    expected="aylo-engine $(cat "$root/VERSION")"
    if [ "$reported" = "$expected" ]; then ok "--version says '$reported'"; else fail "--version says '$reported', VERSION says '$expected'"; fi
fi

# --- Manifest: the hash of every file ----------------------------------------------------------------------------------
if command -v jq >/dev/null 2>&1; then
    if command -v sha256sum >/dev/null 2>&1; then hasher="sha256sum"; else hasher="shasum -a 256"; fi
    bad=0
    listed=0
    for entry in $(jq -r '.files[] | .path + "|" + .sha256' "$root/manifest.json"); do
        file=${entry%%|*}
        want=${entry##*|}
        listed=$((listed + 1))
        have=$($hasher "$root/$file" 2>/dev/null | cut -d ' ' -f 1)
        if [ "$have" != "$want" ]; then
            fail "manifest: $file has hash '$have', the manifest says '$want'"
            bad=$((bad + 1))
        fi
    done
    present=$(find "$root" \( -type f -o -type l \) ! -name manifest.json | wc -l | tr -d ' ')
    [ "$listed" -eq "$present" ] || fail "manifest lists $listed files, the zip holds $present"
    [ "$bad" -eq 0 ] && [ "$listed" -eq "$present" ] && ok "manifest: $listed files match their hash"
else
    warn "jq is not installed: the hashes of the manifest are not checked"
fi

# --- Nothing outside the zip ------------------------------------------------------------------------------------------
check="$here/check-self-contained.sh"
if [ -f "$check" ]; then
    # The whole folder: the tools find their libraries in runtime/, which holds the libraries of the zip.
    if bash "$check" "$root"; then
        ok "the runtime and the tools depend on nothing outside the zip"
    else
        fail "something resolves outside the zip (listed above)"
    fi
else
    warn "$check does not exist: the dependencies of the runtime are not checked"
fi

# --- A server and a headless client -----------------------------------------------------------------------------------
port=$((20000 + $$ % 20000))
game="$root/templates/game"
if [ -x "$runtime" ]; then
    # The server's output is a file, so its stdio is buffered: without line buffering a player joining is not seen
    # until the server stops.
    if command -v stdbuf >/dev/null 2>&1; then
        stdbuf -oL "$runtime" --server --game "$game" --port "$port" > "$work/server.log" 2>&1 &
        watch_connection=1
    else
        "$runtime" --server --game "$game" --port "$port" > "$work/server.log" 2>&1 &
        watch_connection=0
        warn "no stdbuf: the server's output is buffered, so the client connecting is not checked, only that both stay up"
    fi
    server_pid=$!
    sleep 2
    if kill -0 "$server_pid" 2>/dev/null; then
        ok "the server on templates/game is running"
    else
        fail "the server exited: $(cat "$work/server.log")"
        server_pid=""
    fi

    if [ -n "$server_pid" ]; then
        "$runtime" --game "$game" --port "$port" --headless > "$work/client.log" 2>&1 &
        client_pid=$!
        connected=0
        waited=0
        while [ "$waited" -lt 10 ]; do
            sleep 1
            waited=$((waited + 1))
            if ! kill -0 "$client_pid" 2>/dev/null; then
                break
            fi
            if [ "$watch_connection" -eq 1 ] && grep -q "client connected" "$work/server.log"; then
                connected=1
                break
            fi
            if [ "$watch_connection" -eq 0 ] && [ "$waited" -ge 4 ]; then
                break
            fi
        done
        if ! kill -0 "$client_pid" 2>/dev/null; then
            fail "the headless client exited: $(cat "$work/client.log")"
            client_pid=""
        elif [ "$watch_connection" -eq 1 ]; then
            if [ "$connected" -eq 1 ]; then ok "the server saw the client connect"; else fail "the server never saw the client: $(cat "$work/server.log")"; fi
        else
            ok "the headless client is running"
        fi
        if kill -0 "$server_pid" 2>/dev/null; then ok "the server is still running"; else fail "the server stopped: $(cat "$work/server.log")"; fi
    fi
fi

# --- aylo-sign ---------------------------------------------------------------------------------------------------------
mkdir "$work/keys"
if (cd "$work/keys" && "$root/tools/aylo-sign" keygen smoke >/dev/null 2>&1) && [ -s "$work/keys/smoke.pub" ] && [ -s "$work/keys/smoke.key" ]; then
    ok "aylo-sign keygen makes a key pair"
else
    fail "aylo-sign keygen did not make smoke.pub and smoke.key"
fi

echo
if [ "$failures" -eq 0 ]; then
    echo "smoke test passed: $(basename "$zip")"
    exit 0
fi
echo "smoke test FAILED: $failures checks" >&2
exit 1
