# Documentation

Start with the [project README](../README.md). This index covers maintained guides,
reference evidence and preserved historical material.

## Build and use

- [building](building.md) — Build requirements, commands, IDEs and toolchain options.
- [source-updates](source-updates.md) — Confirmed source updates, dependency selection and offline builds.
- [support-matrix](support-matrix.md) — Supported build/runtime matrix and validation limits.
- [supported-data](supported-data.md) — Supported profiles, data fingerprints and shareware/demo downloads.

## Develop and package

- [maintenance](maintenance.md) — Generated/mirrored file ownership and reference-audit findings.

- [testing](testing.md) — Test commands, external-data coverage and maintainer checks.

- [repository-layout](repository-layout.md) — Source directory ownership and file-placement rules.
- [porting-guide](porting-guide.md) — Public host lifecycle, callbacks and integration.
- [architecture](architecture.md) — Engine boundaries, resource/state ownership and preservation design.
- [source-layout](source-layout.md) — Mapping of original source files to modern engine modules.
- [compiler-support](compiler-support.md) — Detailed compiler checkpoint evidence and validation policy.
- [versioning](versioning.md) — Engine revision and ABI version policy.
- [BUILD-NAMING](BUILD-NAMING.md) — Shared package/build identity vocabulary.
- [DISTRIBUTION-CONTENTS](DISTRIBUTION-CONTENTS.md) — Shared binary-package file layout and extension rules.

## History and evidence

- [history](history.md) — Project and repository lineage.
- [provenance](provenance.md) — Routine/source lineage and adaptation evidence.
- [licensing-history](licensing-history.md) — Original/later licensing evidence and component context.
- [reference-index](reference-index.md) — External reference corpus and source authority; local paths are historical context.
- [callgraph-audit](callgraph-audit.md) — Recorded source-parity audit findings; tool usage lives beside the audit tool.

## Archive and proposals

These are preserved records, not current build instructions or fresh validation claims.

- [Completed development plan](archive/development-plan.md)
- [Historical development log](archive/development-log.md)
- [Original 1.4 verification checkpoint](archive/release-1.4-verification.md)
- [.NET project proposal](proposals/wolf3d-dotnet-development-spec.md) — a separate project brief, not this repository's roadmap.

## Nearby documentation

- [Build-script architecture](../scripts/README.md)
- [Linux dispatcher](../scripts/linux/README.md)
- [Visual Studio projects](../ide/visual-studio/README.md)
- [Open Watcom workspace](../ide/open-watcom/README.md)
- [Third-party notices](../THIRD_PARTY.md)
- [Changelog](../CHANGELOG.md)
- [Call-graph tool usage](../tools/CALLGRAPH_AUDIT.md)

## Documentation ownership

The root README is an introduction and quick start. Building owns commands;
source-updates owns Git behavior; support-matrix owns current platform status.
Repository-layout describes tracked source; BUILD-NAMING and
DISTRIBUTION-CONTENTS describe generated output and are mirrored in both repos.
Detailed developer evidence may retain historical checkpoints, explicitly labelled.
Update the owning guide and link it instead of copying its option tables elsewhere.

Run `python tools/WG_DOCS_AUDIT.py` for local Markdown link, anchor, table and
index coverage checks. Python is a maintainer tool, not a build dependency.
