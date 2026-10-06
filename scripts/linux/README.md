# Linux build scripts

GNU Make is the stable Linux build dispatcher and CMake remains the build
graph. Host-specific helper scripts used by Make belong here.

The planned Open Watcom cross-build will run from Linux/Docker and therefore
belongs under this directory even though it will emit a 32-bit DOS library.
