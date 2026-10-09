# Testing and validation

The headless host in platforms/headless/ is an internal deterministic oracle,
not a playable frontend. It supports frame, palette, audio, demo, archive and
gameplay regressions against external original data. Tests live in tests/;
CMakeLists.txt is the authoritative list and conditionally enables data groups.

## Run the engine tests

```sh
make test
make test CC=clang
```

Or use the development CMake presets from [building.md](building.md). To supply
external data explicitly, a modern CMake build can be configured as follows:

```sh
cmake -S . -B build/test-data -DBUILD_TESTING=ON -DWG_BUILD_HEADLESS=ON -DWG_TEST_WL1_PATH=/path/to/WL1
cmake --build build/test-data
ctest --test-dir build/test-data --output-on-failure
```

Additional WG_TEST_*_PATH cache variables cover WL6 variants, SOD, SDM and
SD1/SD2/SD3. See [supported-data.md](supported-data.md) for the corpus hashes.
A passing suite without those paths does not claim coverage of absent data.
Keep proprietary data, generated CONFIG/SAVEGAM files and PPM/WAV captures out
of tracked source; use ignored build directories for local artifacts.

## Maintainer checks

Python is used only by explicitly invoked development tools, not required to
build or run the library:

```sh
python tests/WG_GIT_PREFLIGHT_TEST.py
python tests/WG_DIST_NAMING_TEST.py
python tests/WG_PACKAGE_DOCS_TEST.py
python tools/WG_DOCS_AUDIT.py
```

The preflight tests need Git/Bash; the distribution tests need CMake. See each
script's header for its environment overrides. Source-parity analysis is
explained in [the call-graph tool guide](../tools/CALLGRAPH_AUDIT.md).

Recorded compiler evidence lives in [compiler-support.md](compiler-support.md).
Current platform status lives in [support-matrix.md](support-matrix.md); archived
verification records describe their historical checkpoint, not the current run.
