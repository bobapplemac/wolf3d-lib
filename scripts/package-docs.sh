#!/bin/sh
# Shared by both repositories. Arguments: source root, staged directory, product.
set -eu
root=$1 dist=$2 product=$3
test -f "$dist/DOCS/BUILD.TXT"
test -f "$dist/DOCS/LICENSES/GPL-2.TXT"
cat "$root/packaging/notices/BASE.TXT" > "$dist/DOCS/NOTICES.TXT"
for component in PORTABLE NUKED DBOPL SDL3 DOS32A; do
    include=no
    case "$component" in
        PORTABLE) [ "$product" != wolf3d-portable ] || include=yes ;;
        NUKED) [ ! -f "$dist/DOCS/LICENSES/LGPL-21.TXT" ] || include=yes ;;
        DBOPL) [ ! -f "$dist/DOCS/DBOPL.TXT" ] || include=yes ;;
        *) [ ! -f "$dist/DOCS/LICENSES/$component.TXT" ] || include=yes ;;
    esac
    if [ "$include" = yes ]; then
        cat "$root/packaging/notices/$component.TXT" >> "$dist/DOCS/NOTICES.TXT"
    fi
done
if [ -f "$dist/DOCS/DBOPL.TXT" ]; then
    printf '\n' >> "$dist/DOCS/NOTICES.TXT"
    cat "$dist/DOCS/DBOPL.TXT" >> "$dist/DOCS/NOTICES.TXT"
    rm "$dist/DOCS/DBOPL.TXT"
fi
if [ -f "$dist/DOCS/ENGINE.TXT" ]; then
    printf '\n===== Engine summary =====\n' >> "$dist/DOCS/BUILD.TXT"
    cat "$dist/DOCS/ENGINE.TXT" >> "$dist/DOCS/BUILD.TXT"
    rm "$dist/DOCS/ENGINE.TXT"
fi
cat >> "$dist/README.TXT" <<'EOF'

Documentation
-------------
DOCS/BUILD.TXT records every build option and exact source revisions.
This package contains GPL software; see DOCS/NOTICES.TXT for component
licenses, source information and replacement instructions, and
DOCS/LICENSES for the complete license texts and copyright notices.
Keep these documents with the distribution.
EOF
