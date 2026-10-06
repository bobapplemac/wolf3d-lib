# Build scripts

The root `build.ps1` is the stable modern Windows entry point. With no
arguments it launches `scripts/windows/configure-build.ps1`; explicit
arguments go to the automation-oriented `scripts/windows/invoke-build.ps1`.
Both ultimately delegate to the authoritative CMake presets.

The root `build.sh` follows the same convention on Linux. Its configurator and
thin Make executor live under `scripts/linux/`; direct `make` remains fully
supported. Linux-hosted helpers, including the planned Docker/Open Watcom
cross-build, also belong there.

Root `build.cmd` provides the XP-native guided scanner. Its VC6--VS2005
executor lives at `scripts/windows/legacy/build.cmd`. Visual Studio solutions and projects are
separately grouped by file-format/toolset compatibility under
`ide/visual-studio/<compatibility-band>/`.
