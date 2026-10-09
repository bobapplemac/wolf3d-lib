#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
version=$(sed -n '1p' "$root/VERSION")
build_dir=${WG_OPENWATCOM_BUILD_DIR:-$root/build/openwatcom-dos32}
. "$root/scripts/dist-openwatcom.sh"
dist_dir=${WG_OPENWATCOM_DIST_DIR:-$root/dist/wolf3d-lib_${version}_dos32_x86_${toolchain}}
drivers=${WG_OPENWATCOM_OPL_DRIVERS:-dbopl,silent,adlib}
default_driver=${WG_OPENWATCOM_DEFAULT_OPL:-adlib}
sample_rate=${WG_OPENWATCOM_SAMPLE_RATE:-44100}

case ",$drivers," in
    *,nuked,*|*,dbopl,*|*,silent,*|*,adlib,*) ;;
    *) echo "At least one Open Watcom OPL driver must be selected." >&2; exit 2 ;;
esac
case ",$drivers," in
    *,$default_driver,*) ;;
    *) echo "Default OPL driver '$default_driver' is not compiled in." >&2; exit 2 ;;
esac
objects="$build_dir/objects"
rm -rf "$build_dir" "$dist_dir"
mkdir -p "$objects" "$dist_dir/include" "$dist_dir/DOCS/LICENSES"

defines="-dWOLF3D_STATIC -dWG_DEFAULT_OPL_DRIVER=\"$default_driver\" -dWG_DEFAULT_SAMPLE_RATE=${sample_rate}U"
opl_sources="$root/src/WG_OPL.c"
case ",$drivers," in
    *,nuked,*)
        defines="$defines -dWG_OPL_ENABLE_NUKED=1"
        opl_sources="$opl_sources $root/src/WG_OPL_NUKED.c"
        ;;
esac
case ",$drivers," in
    *,dbopl,*)
        defines="$defines -dWG_OPL_ENABLE_DBOPL=1"
        opl_sources="$opl_sources $root/src/WG_OPL_DBOPL.c $root/third_party/DBOPL/dbopl.c"
        ;;
esac
case ",$drivers," in
    *,silent,*)
        defines="$defines -dWG_OPL_ENABLE_SILENT=1"
        opl_sources="$opl_sources $root/src/WG_OPL_SILENT.c"
        ;;
esac
case ",$drivers," in
    *,adlib,*)
        defines="$defines -dWG_OPL_ENABLE_ADLIB=1"
        opl_sources="$opl_sources $root/src/WG_OPL_ADLIB.c"
        ;;
esac

compile_source()
{
    source=$1
    base=$(basename "$source" .c)
    object="$objects/$base.obj"
    warning_flags="-w4 -we"
    case "$source" in
        "$root"/third_party/*) warning_flags="-w4" ;;
    esac
    echo "Open Watcom C: ${source#$root/}"
    # shellcheck disable=SC2086
    wcc386 -zq -bt=dos -mf -5r -ox -fr $warning_flags \
        -i="$root/include" -i="$root/src" \
        -i="$root/third_party/DBOPL" -i="$root/third_party/Nuked-OPL3" \
        $defines -fo="$object" "$source"
    printf '+%s\n' "$object" >> "$build_dir/wolf3d.lbc"
}

: > "$build_dir/wolf3d.lbc"
while IFS= read -r relative || [ -n "$relative" ]; do
    [ -n "$relative" ] || continue
    compile_source "$root/$relative"
done < "$root/cmake/WGCoreSources.txt"

for source in $opl_sources; do
    compile_source "$source"
done

wlib -q -n "$dist_dir/WOLF3D.LIB" @"$build_dir/wolf3d.lbc"

link_libraries="$dist_dir/WOLF3D.LIB"
case ",$drivers," in
    *,nuked,*)
        echo "Open Watcom C: third_party/Nuked-OPL3/opl3.c"
        wcc386 -zq -bt=dos -mf -5r -ox -fr -w4 \
            -i="$root/third_party/Nuked-OPL3" \
            -fo="$objects/opl3.obj" "$root/third_party/Nuked-OPL3/opl3.c"
        wlib -q -n "$dist_dir/NUKEDOPL.LIB" +"$objects/opl3.obj"
        link_libraries="$link_libraries,$dist_dir/NUKEDOPL.LIB"
        ;;
esac

echo "Open Watcom link check: tests/WOLF3D_CONSUMER.c"
wcc386 -zq -bt=dos -mf -5r -fr -w4 -we \
    -i="$root/include" -dWOLF3D_STATIC \
    -fo="$objects/WGCONSUM.obj" "$root/tests/WOLF3D_CONSUMER.c"
wlink system dos4g option quiet \
    name "$build_dir/WGCONSUM.EXE" \
    file "$objects/WGCONSUM.obj" \
    library "$link_libraries"

cp "$root/include/WOLF3D.h" "$dist_dir/include/WOLF3D.H"
cp "$root/LICENSE" "$dist_dir/DOCS/LICENSES/GPL-2.TXT"
case ",$drivers," in
    *,nuked,*)
        cp "$root/third_party/Nuked-OPL3/LICENSE" \
           "$dist_dir/DOCS/LICENSES/LGPL-21.TXT"
        ;;
esac
case ",$drivers," in
    *,dbopl,*)
        cp "$root/third_party/DBOPL/README.wolf3d-lib.md" \
           "$dist_dir/DOCS/DBOPL.TXT"
        ;;
esac

cat > "$dist_dir/README.TXT" <<EOF
wolf3d-lib $version

Target: 32-bit protected-mode DOS (Open Watcom OMF library)
CPU baseline: Pentium
Compiled OPL drivers: $drivers
Default OPL driver: $default_driver
Preferred PCM sample rate: $sample_rate Hz

Link WOLF3D.LIB into a DOS host that implements the callbacks declared in
include/WOLF3D.H. If the Nuked driver is included, also link NUKEDOPL.LIB.
EOF

echo "Open Watcom DOS32 library staged: $dist_dir"

write_build_info wolf3d-lib dos32 "" "$root"

sh "$root/scripts/package-docs.sh" "$root" "$dist_dir" wolf3d-lib
