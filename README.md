# wolf3d-lib

A portable C99 engine library adapted from Wolfenstein 3D v1.4. It preserves
original gameplay, fixed-point rendering, data formats and timing behind a
small public host callback API.

**Want to play?** Use [wolf3d-portable](https://github.com/bobapplemac/wolf3d-portable),
which provides the GDI, SDL3, Linux KMS/fbdev and DOS VGA applications.
This repository provides the engine SDK and deterministic test host.
Original game data is required and is not distributed here.

## Build the library

```sh
git clone https://github.com/bobapplemac/wolf3d-lib.git
cd wolf3d-lib
```

Run the guided entry point for your build machine:

| Environment | Command |
| --- | --- |
| Linux | `./build.sh` |
| Modern Windows PowerShell | `.\build.ps1` |
| Legacy Windows Command Prompt | `build.cmd` |

The scripts detect available tools and offer source updates for confirmation.
Decline to build your existing checkout. Packages are written under `dist/`;
intermediate output stays under `build/`.

For automation, use `make help` on Linux or the documented CMake presets.
See [building](docs/building.md), [supported compilers/platforms](docs/support-matrix.md),
and [source-update behavior](docs/source-updates.md) for requirements and options.

## Integrate the engine

Include [WOLF3D.h](include/WOLF3D.h), fill `wolf3d_platform_api_t`, and call
`wolf3d_SetPlatform`, `wolf3d_Create`, `wolf3d_Run`, then `wolf3d_Shutdown`.
CMake consumers link `wolf3d::wolf3d`; private `src/` headers are not a host API.

The [porting guide](docs/porting-guide.md) covers lifecycle, input, video and
audio ownership. [Architecture](docs/architecture.md) explains the engine;
[supported data](docs/supported-data.md) lists game profiles, hashes and demo downloads.

## Documentation

Start at the [documentation index](docs/README.md) for the complete guide.
Useful references include [repository layout](docs/repository-layout.md),
[distribution naming](docs/BUILD-NAMING.md), [package contents](docs/DISTRIBUTION-CONTENTS.md),
[versioning](docs/versioning.md), and the [changelog](CHANGELOG.md).

See [LICENSE](LICENSE), [third-party notices](THIRD_PARTY.md), and
[licensing history](docs/licensing-history.md) for terms and provenance.
