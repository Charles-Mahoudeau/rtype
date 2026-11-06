#!/usr/bin/env bash
#
# PostToolUse hook for Edit and Write: formats the edited C and C++ file with clang-format.
#
# Reads the hook's JSON on stdin (the edited path is tool_input.file_path). Exits 0 silently when there is nothing to
# do: no jq, no clang-format, no path, a file that is not C or C++, or a file that no longer exists.

set -u

command -v jq > /dev/null 2>&1 || exit 0
command -v clang-format > /dev/null 2>&1 || exit 0

file="$(jq -r '.tool_input.file_path // empty' 2> /dev/null)" || exit 0
[[ -n "$file" && -f "$file" ]] || exit 0

case "$file" in
    *.cpp | *.hpp | *.tpp | *.c | *.h) ;;
    *) exit 0 ;;
esac

clang-format -i "$file" > /dev/null 2>&1 || true
exit 0
