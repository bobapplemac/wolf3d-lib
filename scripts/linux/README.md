# Linux build scripts

Root `build.sh` launches `configure-build.sh` without arguments and forwards
explicit arguments through `invoke-build.sh` to GNU Make. CMake remains the
build graph, and direct Make use remains supported. The configurator has no
nonstandard runtime dependency and never installs external tools.

The planned Open Watcom cross-build will run from Linux/Docker and therefore
belongs under this directory even though it will emit a 32-bit DOS library.
