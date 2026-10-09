#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
version=$(sed -n '1p' "$root/VERSION")
build_dir=${WG_OPENWATCOM_WINDOWS_BUILD_DIR:-$root/build/openwatcom-win9x-x86}
. "$root/scripts/dist-openwatcom.sh"
dist_dir=${WG_OPENWATCOM_WINDOWS_DIST_DIR:-$root/dist/wolf3d-lib_${version}_win9x_x86_${toolchain}}
drivers=${WG_OPENWATCOM_WINDOWS_OPL_DRIVERS:-dbopl,silent,adlib}
default_driver=${WG_OPENWATCOM_WINDOWS_DEFAULT_OPL:-adlib}
sample_rate=${WG_OPENWATCOM_WINDOWS_SAMPLE_RATE:-48000}

case ",$drivers," in
    *,nuked,*|*,dbopl,*|*,silent,*|*,adlib,*) ;;
    *) echo "At least one Open Watcom Windows OPL driver must be selected." >&2; exit 2 ;;
esac
case ",$drivers," in
    *,$default_driver,*) ;;
    *) echo "Default OPL driver '$default_driver' is not compiled in." >&2; exit 2 ;;
esac

objects="$build_dir/objects"
rm -rf "$build_dir" "$dist_dir"
mkdir -p "$objects" "$dist_dir/include" "$dist_dir/DOCS/LICENSES"

defines="-dWOLF3D_BUILD -dWG_DEFAULT_OPL_DRIVER=\"$default_driver\" -dWG_DEFAULT_SAMPLE_RATE=${sample_rate}U"
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
    echo "Open Watcom Win32 C: ${source#$root/}"
    # shellcheck disable=SC2086
    wcc386 -zq -bt=nt -bd -mf -5r -ox -fr $warning_flags \
        -i="$root/include" -i="$root/src" \
        -i="$root/third_party/DBOPL" -i="$root/third_party/Nuked-OPL3" \
        $defines -fo="$object" "$source"
    printf "file '%s'\n" "$object" >> "$build_dir/wolf3d.lnk"
}

if [ "$(printf '%s' "$drivers" | grep -c nuked || true)" -ne 0 ]; then
    echo "Open Watcom Win32 C: third_party/Nuked-OPL3/opl3.c"
    wcc386 -zq -bt=nt -bd -mf -5r -ox -fr -w4 \
        -i="$root/third_party/Nuked-OPL3" \
        -fo="$objects/opl3.obj" "$root/third_party/Nuked-OPL3/opl3.c"
    cat > "$build_dir/nuked.lnk" <<EOF
system nt_dll
option quiet
option implib='$dist_dir/NUKEDOPL.LIB'
name '$dist_dir/Nuked-OPL3.dll'
file '$objects/opl3.obj'
export OPL3_Generate=OPL3_Generate_
export OPL3_GenerateResampled=OPL3_GenerateResampled_
export OPL3_Reset=OPL3_Reset_
export OPL3_WriteReg=OPL3_WriteReg_
export OPL3_WriteRegBuffered=OPL3_WriteRegBuffered_
export OPL3_GenerateStream=OPL3_GenerateStream_
export OPL3_Generate4ChResampled=OPL3_Generate4ChResampled_
export OPL3_Generate4ChStream=OPL3_Generate4ChStream_
EOF
    wlink @"$build_dir/nuked.lnk"
fi

: > "$build_dir/wolf3d.lnk"
while IFS= read -r relative || [ -n "$relative" ]; do
    [ -n "$relative" ] || continue
    compile_source "$root/$relative"
done < "$root/cmake/WGCoreSources.txt"
for source in $opl_sources; do
    compile_source "$source"
done

cat >> "$build_dir/wolf3d.lnk" <<EOF
system nt_dll
option quiet
option implib='$dist_dir/WOLF3D.LIB'
name '$dist_dir/wolf3d.dll'
EOF
case ",$drivers," in
    *,nuked,*) printf "library '%s'\n" "$dist_dir/NUKEDOPL.LIB" >> "$build_dir/wolf3d.lnk" ;;
esac
wlink @"$build_dir/wolf3d.lnk"

echo "Open Watcom Win32 link check: tests/WOLF3D_CONSUMER.c"
wcc386 -zq -bt=nt -mf -5r -fr -w4 -we \
    -i="$root/include" -fo="$objects/WGCONSUM.obj" \
    "$root/tests/WOLF3D_CONSUMER.c"
wlink system nt option quiet name "$build_dir/WGCONSUM.EXE" \
    file "$objects/WGCONSUM.obj" library "$dist_dir/WOLF3D.LIB"

cp "$root/include/WOLF3D.h" "$dist_dir/include/WOLF3D.H"
cp "$root/LICENSE" "$dist_dir/DOCS/LICENSES/GPL-2.TXT"
case ",$drivers," in
    *,nuked,*) cp "$root/third_party/Nuked-OPL3/LICENSE" "$dist_dir/DOCS/LICENSES/LGPL-21.TXT" ;;
esac
case ",$drivers," in
    *,dbopl,*) cp "$root/third_party/DBOPL/README.wolf3d-lib.md" "$dist_dir/DOCS/DBOPL.TXT" ;;
esac

cat > "$dist_dir/README.TXT" <<EOF
wolf3d-lib $version

Target: 32-bit native Windows PE DLL, Windows 95 compatibility profile
Compiler: Open Watcom 2
CPU baseline: Pentium
Compiled OPL drivers: $drivers
Default OPL driver: $default_driver
Preferred PCM sample rate: $sample_rate Hz

Link WOLF3D.LIB into an Open Watcom Win32 host and place wolf3d.dll beside
the executable. If the Nuked driver is included, Nuked-OPL3.dll must also
remain beside wolf3d.dll; NUKEDOPL.LIB is its consumer import library.
EOF

echo "Open Watcom Win9x library staged: $dist_dir"
sh "$root/tools/WG_WINDOWS_PE_AUDIT.sh" win9x-x86 "$dist_dir"

write_build_info wolf3d-lib win9x "" "$root"

sh "$root/scripts/package-docs.sh" "$root" "$dist_dir" wolf3d-lib
