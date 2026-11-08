#!/usr/bin/env bash
# Downloads package sources whose xmake-repo URL is unreliable into a folder xmake searches before downloading
# (`pkg_searchdirs`). xmake still checks each file's sha256, and when a download fails here it falls back to its own URL:
# this step never fails the build.
#
# OpenSSL: the recipe of openssl3 (needed by libcurl) downloads github.com/openssl/openssl/archive/...zip, a redirect
# that GitHub often answers with 502/504, while codeload.github.com serves the same archive (same sha256).
#
# Usage: prefetch-sources.sh <dir>
set -uo pipefail

dir="${1:?usage: prefetch-sources.sh <dir>}"
mkdir -p "$dir"

fetch() {
    local name="$1" url="$2"
    if curl -fsSL --retry 5 --retry-delay 10 --retry-all-errors --max-time 600 -o "$dir/$name.part" "$url"; then
        mv "$dir/$name.part" "$dir/$name"
        echo "prefetched $name"
    else
        rm -f "$dir/$name.part"
        echo "::warning::could not prefetch $name from $url, xmake will download it itself"
    fi
}

# xmake searches `<package>-<version>.<ext>`: the package name (openssl3), not the archive's.
fetch openssl3-3.6.5.zip https://codeload.github.com/openssl/openssl/zip/refs/tags/openssl-3.6.5

xmake g --pkg_searchdirs="$dir"
