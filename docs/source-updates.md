# Source updates and reproducible builds

The root `build.sh`, `build.ps1`, and `build.cmd` scripts check for newer
published source before building. For a clean older copy, answer **Yes** to
update the source and its required components together, or **No** (the default)
to build your current copy. Local edits, local development commits, and
explicitly selected source versions are preserved. The check follows your
configured branch (normally `main`); it never switches branches.

Portable normally offers to build against the latest compatible `wolf3d-lib`
`main`, including engine fixes published without an application update. The
same confirmation covers application updates and engine updates. The host
independently declares its supported API in `platforms/WG_ENGINE_COMPAT.h`;
an incompatible engine update is skipped with an explanation, and compilation
also rejects an incompatible engine selected manually. Breaking contract
changes must advance the engine API version.

The recorded engine commit remains a reproducible/offline starting point;
third-party components such as SDL3 stay at their recorded versions. An engine
update accepted through these scripts is remembered locally so later checks
can continue updating it. Edits or other custom component selections are
preserved. Build/package versions identify the engine actually selected.

The check needs Git (and Git for Windows or MSYS2 Bash on Windows). Source archives,
unavailable tools, and failed network checks continue with existing sources.
An accepted update that fails stops the build so incomplete dependencies are
not used. Unattended runs never accept updates automatically. Set
`WOLF3D_GIT_CHECK=0` to skip the check entirely, or
`WOLF3D_GIT_INTERACTIVE=0` to check without prompting; PowerShell's
`-NonInteractive` also disables update prompts. Direct Make/CMake and executor
scripts retain their existing behavior. Regression coverage can be run with
`python tests/WG_GIT_PREFLIGHT_TEST.py` (Python 3 and Git/Bash required).

See [building.md](building.md) for build commands and [BUILD-NAMING.md](BUILD-NAMING.md)
for package identity. Direct CMake/Make builds consume the selected working tree;
recorded revisions are restored only when you explicitly update submodules to them.
