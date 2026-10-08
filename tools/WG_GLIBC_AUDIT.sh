#!/bin/sh

set -eu

package_dir=${1:-}
maximum=${2:-2.28}

if [ -z "$package_dir" ] || [ ! -d "$package_dir" ]; then
    printf 'usage: %s PACKAGE_DIRECTORY [MAXIMUM_GLIBC_VERSION]\n' "$0" >&2
    exit 2
fi

overall=
count=0

printf 'glibc symbol audit: %s (maximum GLIBC_%s)\n' "$package_dir" "$maximum"

for candidate in "$package_dir"/*; do
    if ! readelf -h "$candidate" >/dev/null 2>&1; then
        continue
    fi

    count=$((count + 1))
    required=$(
        objdump -T "$candidate" 2>/dev/null \
            | sed -n 's/.*GLIBC_\([0-9][0-9.]*\).*/\1/p' \
            | sort -Vu \
            | tail -n 1
    )
    [ -n "$required" ] || required=none
    printf '  %-30s GLIBC_%s\n' "$(basename "$candidate")" "$required"

    if [ "$required" != none ]; then
        if [ -z "$overall" ]; then
            overall=$required
        else
            overall=$(printf '%s\n%s\n' "$overall" "$required" | sort -Vu | tail -n 1)
        fi
    fi
done

if [ "$count" -eq 0 ]; then
    printf 'error: no ELF files found in %s\n' "$package_dir" >&2
    exit 1
fi

if [ -z "$overall" ]; then
    printf 'error: no glibc symbol versions found in %s\n' "$package_dir" >&2
    exit 1
fi

highest=$(printf '%s\n%s\n' "$overall" "$maximum" | sort -Vu | tail -n 1)
if [ "$highest" != "$maximum" ]; then
    printf 'error: package requires GLIBC_%s, newer than allowed GLIBC_%s\n' \
        "$overall" "$maximum" >&2
    exit 1
fi

printf 'passed: package requires at most GLIBC_%s\n' "$overall"
