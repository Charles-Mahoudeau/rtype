#!/usr/bin/env bash
#
# Checks that a folder runs without anything installed besides the operating system and the GPU driver: every binary in
# it (executable or library, ELF or Mach-O) must find the libraries it links inside the folder, or in the allowlist of
# its platform.
#
# Usage: check-self-contained.sh <folder>
#
# Exits with 0 when the folder is self-contained, with 1 when it is not (one offender per line on the error output),
# with 2 on a usage error. The Windows counterpart is check-self-contained.ps1, with the same output.
#
# What is an offender:
#   - a library a binary links that is neither in the allowlist nor found inside the folder through the binary's own
#     search path (ELF: RPATH/RUNPATH; Mach-O: @rpath, @loader_path, @executable_path);
#   - a search path entry that points outside the folder, such as a build directory;
#   - a symbolic link that points outside the folder.
#
# Libraries opened at run time (dlopen) do not show in a binary's dependencies: the allowlist names the ones we know.
#
# Environment:
#   SELF_CONTAINED_ALLOW  extra allowed library names (Linux, ELF sonames) or path prefixes (macOS), space-separated
#                         shell patterns, for a check on a folder that deliberately relies on one

set -euo pipefail

if [[ $# -ne 1 || ! -d $1 ]]; then
    echo "Usage: $0 <folder>" >&2
    exit 2
fi

root="$(cd "$1" && pwd -P)"
platform="$(uname -s)"
extra_allow="${SELF_CONTAINED_ALLOW:-}"

# What the operating system or the GPU driver provides. Not the C++ runtime (libstdc++, libgcc_s): a machine without a
# compiler may not have it, so it is linked statically or shipped in the folder.
linux_allow=(
    'linux-vdso.so.1' 'linux-gate.so.1' 'ld-linux*.so*' 'ld.so*' 'ld64.so*'
    'libc.so.6' 'libm.so.6' 'libdl.so.2' 'libpthread.so.0' 'librt.so.1' 'libutil.so.1' 'libresolv.so.2'
    # The Vulkan loader is installed with the GPU driver.
    'libvulkan.so.1'
)
# Paths: what the system image holds. @-prefixed ones are resolved against the folder, not allowed by this list.
macos_allow=('/usr/lib/*' '/System/Library/*')

offenders=0
report() {
    echo "$*" >&2
    offenders=$((offenders + 1))
}

# Lexical normalization of an absolute path ("a/b/../c" becomes "a/c"), for paths that may not exist.
normalize() {
    local parts=() part out=""
    local IFS=/
    for part in $1; do
        case "$part" in
        '' | .) ;;
        ..) [[ ${#parts[@]} -gt 0 ]] && unset 'parts[${#parts[@]}-1]' ;;
        *) parts+=("$part") ;;
        esac
    done
    for part in ${parts[@]+"${parts[@]}"}; do
        out="$out/$part"
    done
    echo "${out:-/}"
}

inside() { [[ $1 == "$root" || $1 == "$root"/* ]]; }

matches_any() { # <value> <pattern>...
    local value="$1" pattern
    shift
    for pattern in "$@"; do
        # shellcheck disable=SC2254 # the pattern is meant to be a glob
        case "$value" in $pattern) return 0 ;; esac
    done
    return 1
}

relative() { echo "${1#"$root"/}"; }

# --- symbolic links -------------------------------------------------------------------------------------------------

while IFS= read -r link; do
    target="$(realpath -m "$link" 2>/dev/null || readlink "$link")"
    if ! inside "$target"; then
        report "$(relative "$link"): symbolic link to $target, outside the folder"
    fi
done < <(find "$root" -type l)

# --- ELF (Linux) ----------------------------------------------------------------------------------------------------

is_elf() { [[ "$(head -c 4 "$1" 2>/dev/null | od -An -c | tr -d ' ')" == '177ELF' ]]; }

check_elf() {
    local file="$1" dynamic rpaths=() dirs entry dir resolved name found
    dynamic="$(readelf -d "$file" 2>/dev/null || true)"

    # The search path: RPATH and RUNPATH entries, with $ORIGIN the folder the library is opened from. That is the folder
    # of the file, and the folder of each link to it inside the folder (plugins/ holds links to the libraries next to the
    # executable, and the loader expands $ORIGIN from the path it was given, not from the file the link leads to).
    local origins=() link
    origins+=("$(dirname "$file")")
    while IFS= read -r link; do
        [[ "$(readlink -f "$link")" == "$file" ]] && origins+=("$(dirname "$link")")
    done < <(find "$root" -type l -name "$(basename "$file")")
    while IFS= read -r entry; do
        [[ -z $entry ]] && continue
        IFS=: read -ra dirs <<<"$entry"
        for dir in "${dirs[@]}"; do
            local reachable=0 origin
            for origin in "${origins[@]}"; do
                resolved="${dir//\$ORIGIN/$origin}"
                resolved="${resolved//\$\{ORIGIN\}/$origin}"
                [[ $resolved == /* ]] || resolved="$origin/$resolved"
                resolved="$(normalize "$resolved")"
                if inside "$resolved"; then
                    rpaths+=("$resolved")
                    reachable=1
                fi
            done
            if [[ $reachable -eq 0 ]]; then
                resolved="${dir//\$ORIGIN/${origins[0]}}"
                [[ $resolved == /* ]] || resolved="${origins[0]}/$resolved"
                report "$(relative "$file"): search path entry $(normalize "$resolved") is outside the folder"
            fi
        done
    done < <(sed -n 's/.*(\(RPATH\|RUNPATH\)).*\[\(.*\)\].*/\2/p' <<<"$dynamic")

    while IFS= read -r name; do
        [[ -z $name ]] && continue
        # shellcheck disable=SC2086 # the extra allowlist is a list of patterns
        if matches_any "$name" "${linux_allow[@]}" $extra_allow; then
            continue
        fi
        found=0
        for dir in ${rpaths[@]+"${rpaths[@]}"}; do
            if [[ -e "$dir/$name" ]]; then
                found=1
                break
            fi
        done
        if [[ $found -eq 0 ]]; then
            report "$(relative "$file"): needs $name, which is neither provided by the system nor in the folder" \
                "(reachable through its search path)"
        fi
    done < <(sed -n 's/.*(NEEDED).*\[\(.*\)\].*/\1/p' <<<"$dynamic")
}

# --- Mach-O (macOS) -------------------------------------------------------------------------------------------------

is_macho() { file -b "$1" 2>/dev/null | grep -q 'Mach-O'; }

check_macho() {
    local file="$1" rpaths=() entry dep resolved found base
    base="$(basename "$file")"

    while IFS= read -r entry; do
        [[ -z $entry ]] && continue
        entry="${entry//@loader_path/$(dirname "$file")}"
        entry="${entry//@executable_path/$root}"
        [[ $entry == /* ]] || entry="$(dirname "$file")/$entry"
        resolved="$(normalize "$entry")"
        if inside "$resolved"; then
            rpaths+=("$resolved")
        else
            report "$(relative "$file"): search path entry $resolved is outside the folder"
        fi
    done < <(otool -l "$file" | awk '/cmd LC_RPATH/ {getline; getline; print $2}')

    while IFS= read -r dep; do
        [[ -z $dep ]] && continue
        # A library lists its own install name first.
        [[ "$(basename "$dep")" == "$base" ]] && continue
        # shellcheck disable=SC2086 # the extra allowlist is a list of patterns
        if matches_any "$dep" "${macos_allow[@]}" $extra_allow; then
            continue
        fi
        case "$dep" in
        @rpath/*)
            found=0
            for entry in ${rpaths[@]+"${rpaths[@]}"}; do
                if [[ -e "$entry/${dep#@rpath/}" ]]; then
                    found=1
                    break
                fi
            done
            [[ $found -eq 1 ]] || report "$(relative "$file"): needs $dep, which is not in the folder"
            ;;
        @loader_path/* | @executable_path/*)
            resolved="${dep/@loader_path/$(dirname "$file")}"
            resolved="$(normalize "${resolved/@executable_path/$root}")"
            if ! inside "$resolved" || [[ ! -e $resolved ]]; then
                report "$(relative "$file"): needs $dep, which is not in the folder"
            fi
            ;;
        *) report "$(relative "$file"): needs $dep, outside the folder and not provided by the system" ;;
        esac
    done < <(otool -L "$file" | tail -n +2 | awk '{print $1}')
}

# --- every binary ---------------------------------------------------------------------------------------------------

count=0
while IFS= read -r file; do
    case "$platform" in
    Linux)
        if is_elf "$file"; then
            check_elf "$file"
            count=$((count + 1))
        fi
        ;;
    Darwin)
        if is_macho "$file"; then
            check_macho "$file"
            count=$((count + 1))
        fi
        ;;
    *)
        echo "Unsupported platform $platform: use check-self-contained.ps1 on Windows." >&2
        exit 2
        ;;
    esac
done < <(find "$root" -type f | sort)

if [[ $offenders -gt 0 ]]; then
    echo "$offenders offender(s) in $count binaries of $root." >&2
    exit 1
fi
echo "Self-contained: $count binaries of $root need nothing outside it."
