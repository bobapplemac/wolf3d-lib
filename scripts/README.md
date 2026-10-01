# Build scripts

The root `build.ps1` is the stable modern Windows entry point. Its
implementation lives in `scripts/windows/build.ps1` and delegates to the
authoritative CMake presets.

Linux uses the root Makefile as its human-facing dispatcher; run `make help`.

When an empirically tested legacy Visual Studio band cannot use the modern
dispatcher, its period-appropriate `.cmd` launcher belongs beside that band's
solution under `ide/visual-studio/<compatibility-band>/`, not in this general
scripts directory.
