#!/usr/bin/env bash
#
# Tests check-self-contained.sh on small folders made on the spot: one that is self-contained, and one for each kind of
# offender. Linux and macOS (it needs a C and a C++ compiler on the PATH, CC and CXX name them).
#
# Usage: test-check-self-contained.sh

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
check="$here/check-self-contained.sh"
cxx="${CXX:-c++}"
cc="${CC:-cc}"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

failures=0
expect() { # <name> <expected exit code> <folder> [<text expected in the output>]
    local name="$1" want="$2" folder="$3" text="${4:-}" status=0 output
    output="$("$check" "$folder" 2>&1)" || status=$?
    if [[ $status -ne $want ]] || { [[ -n $text ]] && ! grep -qF -- "$text" <<<"$output"; }; then
        echo "FAIL $name: exit $status (wanted $want), output:" >&2
        echo "$output" | sed 's/^/    /' >&2
        failures=$((failures + 1))
    else
        echo "ok   $name"
    fi
}

if [[ "$(uname -s)" == Darwin ]]; then
    rpath_self='@loader_path'
    rpath_up='@loader_path/..'
    shared='-dynamiclib'
    soname='-install_name @rpath/libanswer.so'
else
    rpath_self='$ORIGIN'
    rpath_up='$ORIGIN/..'
    shared='-shared'
    soname='-Wl,-soname,libanswer.so'
fi

cat >"$work/lib.c" <<'EOF'
int answer() { return 42; }
EOF
cat >"$work/leak.cpp" <<'EOF'
#include <string>
std::string leak() { return std::string("leak"); }
EOF
cat >"$work/main.c" <<'EOF'
int answer();
int main() { return answer() == 42 ? 0 : 1; }
EOF

# A library with nothing to resolve, an executable using it through its search path.
mkdir "$work/ok"
"$cc" $shared -fPIC "$work/lib.c" -o "$work/ok/libanswer.so" $soname -Wl,-rpath,"$rpath_self"
"$cc" "$work/main.c" -o "$work/ok/app" "$work/ok/libanswer.so" -Wl,-rpath,"$rpath_self"
expect "self-contained folder" 0 "$work/ok"

# Run-time paths still inside the folder are fine, even for a folder that is not there yet.
mkdir -p "$work/nested/plugins"
cp "$work/ok/libanswer.so" "$work/ok/app" "$work/nested/"
cp "$work/ok/libanswer.so" "$work/nested/plugins/"
expect "nested folder" 0 "$work/nested"

# A dependency neither in the folder nor in the allowlist: the C++ runtime of the machine.
mkdir "$work/leak"
"$cxx" $shared -fPIC "$work/leak.cpp" -o "$work/leak/libleak.so" -Wl,-rpath,"$rpath_self"
if [[ "$(uname -s)" == Darwin ]]; then
    # libc++ is part of macOS: a Homebrew library stands for the leak there.
    expect "system C++ runtime is allowed on macOS" 0 "$work/leak"
else
    expect "dependency on the system C++ runtime" 1 "$work/leak" "needs libstdc++.so.6"
fi

# A dependency that is in the folder, but that the binary cannot reach.
mkdir -p "$work/unreachable"
cp "$work/ok/libanswer.so" "$work/unreachable/"
"$cc" "$work/main.c" -o "$work/unreachable/app" "$work/ok/libanswer.so"
if [[ "$(uname -s)" == Linux ]]; then
    expect "library in the folder but outside the search path" 1 "$work/unreachable" "needs libanswer.so"
fi

# A search path that leaves the folder, such as a build directory.
mkdir "$work/rpath"
cp "$work/ok/libanswer.so" "$work/rpath/"
"$cc" "$work/main.c" -o "$work/rpath/app" "$work/ok/libanswer.so" -Wl,-rpath,"$work/ok" -Wl,-rpath,"$rpath_self"
expect "search path outside the folder" 1 "$work/rpath" "outside the folder"

# A search path going up from a nested folder out of it.
mkdir -p "$work/escape/plugins"
cp "$work/ok/libanswer.so" "$work/escape/"
"$cc" $shared -fPIC "$work/lib.c" -o "$work/escape/plugins/libplugin.so" -Wl,-rpath,"$rpath_up/.."
expect "relative search path leaving the folder" 1 "$work/escape" "outside the folder"

# A symbolic link to a file outside the folder (the build's plugins/ folder is made of them).
mkdir "$work/link"
cp "$work/ok/libanswer.so" "$work/ok/app" "$work/link/"
ln -s "$work/ok/libanswer.so" "$work/link/libother.so"
expect "symbolic link leaving the folder" 1 "$work/link" "symbolic link"

# The extra allowlist.
if [[ "$(uname -s)" == Linux ]]; then
    status=0
    SELF_CONTAINED_ALLOW='libstdc++.so.6 libgcc_s.so.1' "$check" "$work/leak" >/dev/null 2>&1 || status=$?
    if [[ $status -eq 0 ]]; then echo "ok   SELF_CONTAINED_ALLOW"; else
        echo "FAIL SELF_CONTAINED_ALLOW: exit $status" >&2
        failures=$((failures + 1))
    fi
fi

# Usage errors.
status=0
"$check" >/dev/null 2>&1 || status=$?
if [[ $status -eq 2 ]]; then echo "ok   no argument"; else echo "FAIL no argument: exit $status" >&2; failures=$((failures + 1)); fi
status=0
"$check" "$work/missing" >/dev/null 2>&1 || status=$?
if [[ $status -eq 2 ]]; then echo "ok   missing folder"; else echo "FAIL missing folder: exit $status" >&2; failures=$((failures + 1)); fi

if [[ $failures -gt 0 ]]; then
    echo "$failures test(s) failed." >&2
    exit 1
fi
echo "All tests passed."
