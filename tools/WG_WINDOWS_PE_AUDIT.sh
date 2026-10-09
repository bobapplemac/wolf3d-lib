#!/bin/sh
set -eu

profile=${1:?usage: WG_WINDOWS_PE_AUDIT.sh PROFILE DIRECTORY}
directory=${2:?usage: WG_WINDOWS_PE_AUDIT.sh PROFILE DIRECTORY}

case "$profile" in
    llvm-*-x86) objdump=${OBJDUMP:-/usr/bin/i686-w64-mingw32-objdump}; machine='pei-i386' ;;
    llvm-*-x64) objdump=${OBJDUMP:-/usr/bin/x86_64-w64-mingw32-objdump}; machine='pei-x86-64' ;;
    *-x86|win9x-x86|xp-x86) objdump=${OBJDUMP:-i686-w64-mingw32-objdump}; machine='pei-i386' ;;
    *-x64|win10-x64) objdump=${OBJDUMP:-x86_64-w64-mingw32-objdump}; machine='pei-x86-64' ;;
    *) echo "Unknown PE audit profile: $profile" >&2; exit 2 ;;
esac

command -v "$objdump" >/dev/null 2>&1 || {
    echo "PE audit tool not found: $objdump" >&2
    exit 2
}
[ -d "$directory" ] || { echo "PE package not found: $directory" >&2; exit 2; }

found=0
for file in "$directory"/*.exe "$directory"/*.dll; do
    [ -f "$file" ] || continue
    found=1
    headers=$($objdump -f -p "$file")
    echo "$headers" | grep -q "file format $machine" || {
        echo "Unexpected PE architecture: $file" >&2
        exit 1
    }
    if echo "$headers" | grep -Eqi 'DLL Name: (libgcc_s|libwinpthread|libstdc\+\+)'; then
        echo "Non-redistributed GNU runtime dependency: $file" >&2
        exit 1
    fi
    case "$profile" in
        win9x-x86)
            case "$file" in
                *.exe)
                    echo "$headers" | grep -Eq 'Subsystem[[:space:]]+00000003' || {
                        echo "Win9x launchers must inherit a console: $file" >&2
                        exit 1
                    }
                    ;;
            esac
            if echo "$headers" | grep -Eqi \
                'DLL Name: (api-ms-|ucrtbase)|GetMonitorInfoW|GetWindowLongPtrW|RegisterRawInputDevices|GetRawInputData|CommandLineToArgvW'; then
                echo "Post-Win9x dependency found: $file" >&2
                exit 1
            fi
            ;;
        *msvcrt*|*xp*|*win7*)
            if echo "$headers" | grep -Eqi 'DLL Name: (api-ms-win-crt|ucrtbase)'; then
                echo "UCRT dependency found in an MSVCRT profile: $file" >&2
                exit 1
            fi
            ;;
    esac
done

[ "$found" -eq 1 ] || { echo "No PE files found in $directory" >&2; exit 1; }
echo "PE audit passed: $profile ($directory)"
