# Build scripts

The root `build.ps1` is the stable modern Windows entry point. Its
implementation lives in `scripts/windows/build.ps1` and delegates to the
authoritative CMake presets.

Linux uses the root Makefile as its human-facing dispatcher; run `make help`.
Linux-hosted helper scripts, including the planned Docker/Open Watcom
cross-build, live under `scripts/linux/`.

The XP-native VC6--VS2005 dispatcher lives at
`scripts/windows/legacy/build.cmd`. Visual Studio solutions and projects are
separately grouped by file-format/toolset compatibility under
`ide/visual-studio/<compatibility-band>/`.
