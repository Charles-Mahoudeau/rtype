#!/usr/bin/env bash
# Ad-hoc signs, or checks the signature of, every Mach-O file of a folder.
#
#   codesign-macos.sh <folder>             signs (codesign -s -) every binary and library of the folder
#   codesign-macos.sh --verify <folder>    fails if one of them has no valid signature
#
# An arm64 binary without a valid signature is killed by the kernel, and `strip` invalidates the one the linker made: the
# packaging runs this after stripping and before it writes manifest.json and the zip, and the release workflow runs
# --verify on the extracted zip. An ad-hoc signature is not an identity: Gatekeeper still asks the user to approve a
# downloaded zip (docs/release/signing.md).
set -euo pipefail

mode=sign
if [[ "${1:-}" == "--verify" ]]; then
  mode=verify
  shift
fi

if [[ $# -ne 1 || ! -d "$1" ]]; then
  echo "Usage: $0 [--verify] <folder>" >&2
  exit 2
fi
folder="$1"

if ! command -v codesign > /dev/null; then
  echo "codesign not found: this script runs on macOS." >&2
  exit 2
fi

# Regular files only: a symbolic link is skipped, the file it points to is signed through its own path. Data files the
# runtime carries are skipped by name, the rest is asked of `file`.
mach_o_files() {
  find "$folder" -type f ! -name '*.json' ! -name '*.md' ! -name '*.luau' -print0 \
    | while IFS= read -r -d '' file; do
        case "$(file -b "$file")" in
          Mach-O*) printf '%s\0' "$file" ;;
        esac
      done
}

count=0
failed=0
while IFS= read -r -d '' file; do
  count=$((count + 1))
  if [[ "$mode" == "sign" ]]; then
    codesign --force --sign - --timestamp=none "$file"
  elif ! codesign --verify --strict "$file" 2> /dev/null; then
    echo "Not signed or signature invalid: ${file}" >&2
    failed=$((failed + 1))
  fi
done < <(mach_o_files)

if [[ "$count" -eq 0 ]]; then
  echo "No Mach-O file in ${folder}: wrong folder?" >&2
  exit 1
fi
if [[ "$failed" -ne 0 ]]; then
  echo "${failed} of ${count} files without a valid signature." >&2
  exit 1
fi
if [[ "$mode" == "sign" ]]; then
  echo "Signed ${count} files."
else
  echo "${count} files have a valid signature."
fi
