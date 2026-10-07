# Linux build scripts

Root `build.sh` launches `configure-build.sh` without arguments and forwards
explicit arguments through `invoke-build.sh` to GNU Make. CMake remains the
build graph, and direct Make use remains supported. The configurator has no
nonstandard runtime dependency and never installs external tools.

The Open Watcom cross-build runs from Linux/Docker and therefore belongs under
this directory even though it emits a 32-bit DOS library. Use
`make openwatcom` or select the DOS32 SDK in the root `./build.sh` wizard.
`openwatcom/build-library.sh` is the deterministic container-side backend;
it is not intended to install or discover a host Open Watcom environment.
