#!/usr/bin/env bash
set -e
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
executor="$root/scripts/linux/invoke-build.sh"

choose() {
    local prompt=$1; shift
    local options=("$@") answer i
    printf '\n%s\n' "$prompt" >&2
    for ((i=0; i<${#options[@]}; ++i)); do
        if [ "$i" -eq 0 ]; then
            printf '  %d. %s (recommended)\n' "$((i + 1))" "${options[$i]}" >&2
        else
            printf '  %d. %s\n' "$((i + 1))" "${options[$i]}" >&2
        fi
    done
    while :; do
        read -r -p 'Selection: ' answer
        answer=${answer:-1}
        if [[ $answer =~ ^[0-9]+$ ]] && ((answer >= 1 && answer <= ${#options[@]})); then
            printf '%s' "${options[$((answer - 1))]}"
            return
        fi
        printf 'Enter one of the listed numbers.\n' >&2
    done
}

ready() { command -v "$1" >/dev/null 2>&1; }
printf 'wolf3d-lib guided Linux build\n'
printf 'Scanning supported compilers and build tools...\n\n'
for tool in gcc clang cmake make docker git; do
    if ready "$tool"; then printf '  %-8s ready - %s\n' "$tool" "$(command -v "$tool")"
    else printf '  %-8s not found\n' "$tool"; fi
done

targets=()
labels=()
if ready cmake && ready make; then
    targets+=(library-release test build clean)
    labels+=('native library distribution' 'validation tests' 'development build' 'clean local build outputs')
fi
if ready docker && ready make; then
    targets+=(portable-library-release musl-library-release)
    labels+=('portable glibc 2.28 distribution (Docker)' 'relocatable musl distribution (Docker)')
fi
if [ ${#targets[@]} -eq 0 ]; then
    printf '\nNo usable build path was detected. See docs/building.md for prerequisites.\n' >&2
    exit 2
fi

label=$(choose 'What would you like to produce?' "${labels[@]}")
target=''
for ((i=0; i<${#labels[@]}; ++i)); do
    [ "${labels[$i]}" = "$label" ] && target=${targets[$i]}
done

compiler=gcc
if [[ $target != portable-* && $target != musl-* && $target != clean ]]; then
    compilers=()
    ready gcc && compilers+=(gcc)
    ready clang && compilers+=(clang)
    [ ${#compilers[@]} -gt 0 ] || { printf 'No supported C compiler was found.\n' >&2; exit 2; }
    compiler=$(choose 'C compiler' "${compilers[@]}")
fi

drivers=all
default_opl=nuked
sample_rate=48000
if [[ $target != clean ]]; then
    drivers=$(choose 'Compiled OPL drivers' all nuked-dbopl nuked-silent dbopl-silent nuked dbopl silent)
    if [ "$drivers" = all ]; then
        default_opl=$(choose 'Default OPL driver' nuked dbopl silent)
    else
        IFS=- read -r -a defaults <<< "$drivers"
        default_opl=$(choose 'Default OPL driver' "${defaults[@]}")
    fi
    sample_rate=$(choose 'Preferred PCM sample rate' 48000 44100)
fi
read -r -p 'Parallel jobs (blank lets the build tool decide): ' jobs

opl_drivers=${drivers//-/,}
[ "$drivers" = all ] && opl_drivers=nuked,dbopl,silent
args=("$target" "CC=$compiler" "OPL_DRIVERS=$opl_drivers" "OPL_DEFAULT=$default_opl" "SAMPLE_RATE=$sample_rate")
[ -n "$jobs" ] && args+=("JOBS=$jobs")
printf '\nBuild plan:\n  Target:      %s\n  Compiler:    %s\n  OPL drivers: %s (default: %s)\n  Sample rate: %s Hz\n' "$target" "$compiler" "$opl_drivers" "$default_opl" "$sample_rate"
printf '\nReproducible command:\n  ./scripts/linux/invoke-build.sh'
printf ' %q' "${args[@]}"
printf '\n\n'
read -r -p 'Run this build now? [Y/n] ' answer
if [[ -n $answer && ! $answer =~ ^[Yy] ]]; then exit 0; fi
exec "$executor" "${args[@]}"
