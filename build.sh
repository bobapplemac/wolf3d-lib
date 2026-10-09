#!/usr/bin/env bash
set -e
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
bash "$root/scripts/git-preflight.sh" "$root"
if [ "$#" -eq 0 ]; then
    exec "$root/scripts/linux/configure-build.sh"
fi
exec "$root/scripts/linux/invoke-build.sh" "$@"
