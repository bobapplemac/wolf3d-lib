# Shared by the Open Watcom release scripts. Mirrored in both repositories.
# Source this after setting root/version, before choosing dist_dir.
compiler_banner=$(wcc386 2>&1 || :)
compiler_version=$(printf '%s\n' "$compiler_banner" | sed -n 's/.*Version \([0-9][0-9.]*\).*/\1/p' | head -n 1)
compiler_major=${compiler_version%%.*}
case "$compiler_major" in
    ''|*[!0-9]*) echo 'Cannot detect Open Watcom compiler version' >&2; exit 2 ;;
esac
toolchain=openwatcom$compiler_major

write_build_info()
{
    product=$1 platform=$2 backend=$3 engine=$4
    {
        printf 'Product: %s\nVersion: %s\nPlatform baseline: %s\nArchitecture: x86\nBackend: %s\n' "$product" "$version" "$platform" "$backend"
        printf 'Toolchain: %s\nCompiler version: %s\nConfiguration: Release\nCPU baseline: Pentium\n' "$toolchain" "$compiler_version"
        printf 'Compiler banner: %s\n' "$compiler_banner"
        printf 'OPL drivers: %s\nDefault OPL driver: %s\nPreferred sample rate: %s Hz\n' "$drivers" "$default_driver" "$sample_rate"
        printf 'Build directory: %s\nDistribution directory: %s\n' "$build_dir" "$dist_dir"
        printf 'Compiler definitions: %s\nLink libraries: %s\n' "${defines:-see recipe below}" "${link_libraries:-see recipe below}"
        printf 'WATCOM: %s\nINCLUDE: %s\nLIB: %s\n' "${WATCOM:-}" "${INCLUDE:-}" "${LIB:-}"
        printf 'WCC386: %s\nWLINK: %s\nWLIB: %s\n' "${WCC386:-}" "${WLINK:-}" "${WLIB:-}"
        for source in "$root" "$engine"; do
            commit=$(git -C "$source" rev-parse HEAD 2>/dev/null || printf unknown)
            printf 'Source %s: %s\n' "$source" "$commit"
            git -C "$source" diff --quiet 2>/dev/null || printf 'Source has local changes or Git status is unavailable.\n'
        done
        # All fixed compiler/linker switches and source selections are part of
        # the recipe. Preserve it alongside the resolved options above.
        printf '\n===== Build recipe: %s =====\n' "$0"
        cat "$0"
        if [ "$product" = wolf3d-portable ]; then
            printf '\n===== Engine build configuration =====\n'
            if [ -f "$library_dist/DOCS/BUILD.TXT" ]; then
                cat "$library_dist/DOCS/BUILD.TXT"
            elif [ -f "$library_dist/BUILD-INFO.txt" ]; then
                cat "$library_dist/BUILD-INFO.txt"
            else
                # Older compatible engines predate DOCS/BUILD.TXT. Preserve
                # their recipe; resolved audio options above were passed in.
                case "$platform" in
                    dos32) recipe=build-library.sh ;;
                    win9x) recipe=build-windows-library.sh ;;
                esac
                cat "$engine/scripts/linux/openwatcom/$recipe"
            fi
        fi
    } > "$dist_dir/DOCS/BUILD.TXT"
}
