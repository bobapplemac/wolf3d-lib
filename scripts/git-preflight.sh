#!/usr/bin/env bash
# Refresh remote information freely; source changes require explicit consent.
set -u
root=${1:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
[ "${WOLF3D_GIT_CHECK:-1}" != 0 ] || exit 0
command -v git >/dev/null 2>&1 || { echo 'Git not found; building existing sources.'; exit 0; }
cd -- "$root" || exit 1
[ -e .git ] || { echo 'Source export: update check skipped.'; exit 0; }
git rev-parse --is-inside-work-tree >/dev/null || { echo 'Git could not inspect this checkout. Resolve the Git error above before building.' >&2; exit 1; }
export GIT_TERMINAL_PROMPT=0
export GIT_SSH_COMMAND="${GIT_SSH_COMMAND:-ssh -o BatchMode=yes -o ConnectTimeout=10}"
confirm() {
    local answer
    if [ "${WOLF3D_GIT_INTERACTIVE:-auto}" = 0 ] ||
       { [ "${WOLF3D_GIT_INTERACTIVE:-auto}" != 1 ] && [ ! -t 0 ]; }; then
        echo 'Noninteractive build: keeping existing sources.'
        return 1
    fi
    printf '%s [y/N] ' 'Newer source or required components are available. Update before building?'
    read -r answer || return 1
    answer=${answer%$'\r'}
    case "$answer" in y|Y|yes|YES|Yes) return 0;; *) return 1;; esac
}
fetch_remote() {
    git -c http.lowSpeedLimit=1 -c http.lowSpeedTime=15 fetch --quiet --no-tags "$1"
}
# A previously accepted engine update is normal, not local development work.
# All other edits, commits, and custom component selections are preserved.
clean_tree() {
    local status managed current
    status=$(git status --porcelain --untracked-files=normal --ignore-submodules=none) || return 2
    [ -n "$status" ] || return 0
    [ "$status" = ' M lib/wolf3d' ] || return 1
    managed=$(git config --local --get wolf3d.buildEngineRevision || true)
    current=$(git -C lib/wolf3d rev-parse HEAD) || return 2
    [ -n "$managed" ] && [ "$current" = "$managed" ] || return 1
    status=$(git -C lib/wolf3d status --porcelain --untracked-files=normal --ignore-submodules=none) || return 2
    [ -z "$status" ]
}
update_pins() {
    git submodule sync --quiet --recursive &&
    git submodule update --quiet --init --recursive || {
        echo 'Component download failed. Build stopped; see the Git error above.' >&2
        exit 1
    }
}
branch=$(git symbolic-ref --quiet --short HEAD 2>/dev/null || true)
if [ -z "$branch" ]; then
    echo 'This copy uses a specific source version; building it as selected.'
    exit 0
fi
upstream=$(git rev-parse --abbrev-ref --symbolic-full-name '@{upstream}' 2>/dev/null || true)
remote=$(git config --get "branch.$branch.remote" || true)
if [ -z "$upstream" ] || [ -z "$remote" ]; then
    echo "No published update source is configured for $branch; building this copy."
    exit 0
fi
echo "Checking for source updates ($branch)..."
if ! fetch_remote "$remote"; then
    echo 'Could not check for updates. Building existing sources; a newer version may be available.'
    exit 0
fi
original_head=$(git rev-parse HEAD) || exit 1
target=$(git rev-parse '@{upstream}') || exit 1
counts=$(git rev-list --left-right --count "HEAD...$target") || exit 1
read -r ahead behind <<< "$counts"
clean_tree
tree_status=$?
if [ "$tree_status" -eq 2 ]; then
    echo 'Git could not inspect the source or a component. Resolve the Git error above before building.' >&2
    exit 1
fi
if [ "$tree_status" -ne 0 ]; then
    echo 'Local changes detected. Building your current copy and keeping your work.'
    exit 0
fi
if [ "$ahead" -gt 0 ]; then
    echo 'Local development history detected. Building your current copy and keeping your work.'
    exit 0
fi
approved=0
dependencies=$(git submodule status --recursive) || exit 1
if [ "$behind" -gt 0 ] || printf '%s\n' "$dependencies" | grep -q '^-'; then
    if confirm; then
        clean_tree || { echo 'Source changed during confirmation; build stopped.' >&2; exit 1; }
        if [ "$(git rev-parse HEAD)" != "$original_head" ] ||
           [ "$(git symbolic-ref --quiet --short HEAD)" != "$branch" ]; then
            echo 'Source selection changed during confirmation; build stopped.' >&2
            exit 1
        fi
        git merge --quiet --ff-only "$target" || exit 1
        update_pins
        approved=1
        echo 'Source and required components updated.'
    else
        echo 'Update declined; building existing sources.'
        exit 0
    fi
else
    echo 'Your application source is up to date.'
fi

# Portable follows engine main independently; third-party components stay pinned.
compat=platforms/WG_ENGINE_COMPAT.h
[ -f "$compat" ] && [ -f lib/wolf3d/include/WOLF3D.h ] || exit 0
expected=$(sed -n 's/^#define W3P_ENGINE_API_VERSION \([0-9][0-9]*\)U.*/\1/p' "$compat")
[ -n "$expected" ] || { echo 'Cannot read engine compatibility requirement; build stopped.' >&2; exit 1; }
echo 'Checking for engine updates...'
if ! (cd lib/wolf3d && fetch_remote origin); then
    echo 'Could not check engine updates; keeping the existing engine.'
    exit 0
fi
engine_target=$(git -C lib/wolf3d rev-parse --verify 'refs/remotes/origin/main^{commit}' 2>/dev/null || true)
[ -n "$engine_target" ] || { echo 'Published engine source unavailable; keeping the existing engine.'; exit 0; }
engine_current=$(git -C lib/wolf3d rev-parse HEAD) || exit 1
[ "$engine_target" != "$engine_current" ] || { echo 'Your engine is up to date.'; exit 0; }
actual=$(git -C lib/wolf3d show "$engine_target:include/WOLF3D.h" | sed -n 's/^#define WOLF3D_PLATFORM_API_VERSION \([0-9][0-9]*\)U.*/\1/p')
if [ "$actual" != "$expected" ]; then
    echo "The newest engine requires a different API (engine ${actual:-unknown}, application $expected). Keeping the existing engine; a compatible application update is needed."
    exit 0
fi
if ! git -C lib/wolf3d merge-base --is-ancestor "$engine_current" "$engine_target"; then
    echo 'The engine has a different development history; keeping your selected version.'
    exit 0
fi
if [ "$approved" = 0 ] && ! confirm; then
    echo 'Update declined; keeping the existing engine.'
    exit 0
fi
if ! clean_tree || [ "$(git -C lib/wolf3d rev-parse HEAD)" != "$engine_current" ] ||
   [ "$(git rev-parse HEAD)" != "$target" ] ||
   [ "$(git symbolic-ref --quiet --short HEAD)" != "$branch" ]; then
    echo 'Source changed during confirmation; build stopped.' >&2
    exit 1
fi
git -C lib/wolf3d checkout --quiet --detach "$engine_target" || exit 1
(cd lib/wolf3d && update_pins) || exit 1
git config --local wolf3d.buildEngineRevision "$engine_target" || exit 1
echo "Engine updated to latest compatible main (${engine_target:0:12}, API $expected)."
