#!/usr/bin/env bash
#
# Checks the C and C++ files changed between a base commit and HEAD with clang-format and clang-tidy.
#
# Usage: lint.sh <base>
#
# The diff is `<base>...HEAD`, so for a pull request in a stack whose base is the stack's base (main), it covers the
# whole stack, not only the top pull request. Every changed file is checked in full, not only its changed lines.
# clang-tidy needs compile_commands.json at the repository root, and skips the files of the folders whose
# .clang-tidy disables every check (tests, examples, vendored code).
#
# Environment:
#   CLANG_FORMAT, CLANG_TIDY  the tools to run (default: clang-format, clang-tidy)
#   LINT_JOBS                 how many clang-tidy processes run at once (default: the number of CPUs)
#   GITHUB_STEP_SUMMARY       set by GitHub Actions: the report is appended to it
#
# Exits with 1 when a file is not formatted or has a clang-tidy diagnostic, and prints a GitHub annotation for each.

# The single-quoted `$0` (bash -c) and backquotes (Markdown) are not meant to expand.
# shellcheck disable=SC2016

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <base>" >&2
    exit 2
fi

base="$1"
clang_format="${CLANG_FORMAT:-clang-format}"
clang_tidy="${CLANG_TIDY:-clang-tidy}"
jobs="${LINT_JOBS:-$(nproc)}"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# Written to a file first so that `set -e` stops the script when git diff fails, and NUL-delimited so that Git does not
# quote the paths with unusual characters.
git diff -z --name-only --diff-filter=d "${base}...HEAD" -- \
    '*.c' '*.h' '*.cpp' '*.hpp' '*.cc' '*.hh' '*.cxx' '*.hxx' ':!.github' >"$work/files.txt"
mapfile -d '' -t files <"$work/files.txt"

echo "Changed files since ${base}: ${#files[@]}"
if [[ ${#files[@]} -eq 0 ]]; then
    echo "## Lint" >>"$summary"
    echo "No C or C++ file changed since \`${base}\`." >>"$summary"
    exit 0
fi

# Prints a GitHub annotation for each `path:line:col: warning|error: message` line of a tool's output.
annotate() {
    local level message
    while IFS= read -r line; do
        if [[ $line =~ ^(.+):([0-9]+):([0-9]+):\ (warning|error):\ (.*)$ ]]; then
            level="${BASH_REMATCH[4]}"
            message="${BASH_REMATCH[5]//%/%25}"
            echo "::${level} file=${BASH_REMATCH[1]#"$PWD/"},line=${BASH_REMATCH[2]},col=${BASH_REMATCH[3]}::${message}"
        fi
    done
}

# clang-format

format_failed=()
for file in "${files[@]}"; do
    if ! "$clang_format" --dry-run --Werror --style=file "$file" >"$work/format.log" 2>&1; then
        format_failed+=("$file")
        cat "$work/format.log"
        annotate <"$work/format.log"
    fi
done

# clang-tidy, in parallel: each file writes its own log, kept only when it reports something.

if ! command -v "$clang_tidy" >/dev/null; then
    echo "clang-tidy not found: ${clang_tidy}" >&2
    exit 2
fi

tidy_files=()
for file in "${files[@]}"; do
    if "$clang_tidy" --list-checks "$file" >/dev/null 2>&1; then
        tidy_files+=("$file")
    fi
done
echo "Files checked by clang-tidy: ${#tidy_files[@]} (the others are in folders where every check is disabled)"

# Only the diagnostics located in the checked file count (and their notes): the ones clang-tidy reports in the headers
# it includes (HeaderFilterRegex is '.*') belong to those headers, which are checked on their own when they change.
run_tidy() {
    local file="$1" log
    log="$LINT_WORK/tidy/$(printf '%s' "$file" | tr '/' '_').log"
    "$LINT_CLANG_TIDY" -p . --quiet "$file" 2>&1 | awk -v file="$file" -v root="$PWD/" '
        /^[^ ].*:[0-9]+:[0-9]+: (warning|error|note): / {
            if ($0 !~ /:[0-9]+:[0-9]+: note: /) {
                path = $0
                sub(/:[0-9]+:[0-9]+: (warning|error): .*$/, "", path)
                if (index(path, root) == 1) path = substr(path, length(root) + 1)
                keep = (path == file)
            }
        }
        /^[0-9]+ (warning|error)s? generated/ { next }
        keep { print }
    ' >"$log.out" || true
    if grep -qE ':[0-9]+:[0-9]+: (warning|error):' "$log.out"; then
        { echo "$file"; cat "$log.out"; } >"$log"
    fi
    rm "$log.out"
}
export -f run_tidy
export LINT_WORK="$work" LINT_CLANG_TIDY="$clang_tidy"
mkdir "$work/tidy"
if [[ ${#tidy_files[@]} -gt 0 ]]; then
    printf '%s\0' "${tidy_files[@]}" | xargs -0 -n 1 -P "$jobs" bash -c 'run_tidy "$0"'
fi

tidy_failed=()
for log in "$work"/tidy/*.log; do
    [[ -e $log ]] || continue
    tidy_failed+=("$(head -n 1 "$log")")
    tail -n +2 "$log"
    tail -n +2 "$log" | annotate
done

# Report

{
    echo "## Lint"
    echo
    echo "Checked ${#files[@]} changed file(s) since \`${base}\` (${#tidy_files[@]} with clang-tidy)."
    echo
    if [[ ${#format_failed[@]} -eq 0 ]]; then
        echo "- clang-format: all files are formatted."
    else
        echo "- clang-format: ${#format_failed[@]} file(s) not formatted:"
        printf '  - `%s`\n' "${format_failed[@]}"
    fi
    if [[ ${#tidy_failed[@]} -eq 0 ]]; then
        echo "- clang-tidy: no diagnostic."
    else
        echo "- clang-tidy: ${#tidy_failed[@]} file(s) with diagnostics:"
        printf '  - `%s`\n' "${tidy_failed[@]}"
    fi
} | tee -a "$summary"

if [[ ${#format_failed[@]} -gt 0 || ${#tidy_failed[@]} -gt 0 ]]; then
    exit 1
fi
