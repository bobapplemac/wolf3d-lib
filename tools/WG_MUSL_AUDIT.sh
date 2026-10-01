#!/bin/sh
set -eu

package_dir=${1:?usage: WG_MUSL_AUDIT.sh PACKAGE_DIR}

if [ ! -d "$package_dir" ]; then
    printf '%s\n' "musl audit: package directory not found: $package_dir" >&2
    exit 2
fi

objects=$(find "$package_dir" -type f \( -name '*.so' -o -name '*.so.*' \))
if [ -z "$objects" ]; then
    printf '%s\n' "musl audit: no ELF shared libraries found in $package_dir" >&2
    exit 2
fi

for object in $objects; do
    if readelf --version-info "$object" 2>/dev/null | grep -q 'GLIBC_'; then
        printf '%s\n' "musl audit: glibc symbol reference found in $object" >&2
        exit 1
    fi
    if ! readelf -d "$object" >/dev/null 2>&1; then
        printf '%s\n' "musl audit: unreadable ELF object: $object" >&2
        exit 1
    fi
    printf '%s\n' "musl audit: $object"
    readelf -d "$object" | awk '/NEEDED/ { print "  " $0 }'
done

printf '%s\n' 'musl audit: no glibc symbol versions found'
