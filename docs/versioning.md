# Versioning

Wolfenstein 3D's historically meaningful product version remains `1.4`. This
project adds a monotonically increasing third component to identify each
committed library revision:

```text
1.4.REVISION
```

The root `VERSION` file is authoritative. CMake, GNU Make, generated package
metadata, and distribution folder names consume it directly. The configured
build fails when the file is malformed or its version has no corresponding
heading in `CHANGELOG.md`; `make check-version` performs the same validation
without compiling the project.

Beginning with 1.4.17, every commit to the library main branch must advance
the revision inside that same commit. A separate version-only follow-up commit
is not used, because it would itself require another revision. Concurrent work
must be rebased and assigned the next available revision before it reaches
main.

Revisions 1.4.1 through 1.4.16 were assigned retrospectively to the sixteen
dated milestones following the original 1.4.0 promotion. This documentary
backfill does not rewrite existing Git history.

The product revision is independent of shared-library ABI compatibility. A
future Linux SONAME changes only for an incompatible public ABI change, not
for each product revision.

The curated public reconstruction from the original id Software ancestor is a
history presentation of the already released 1.4.52 checkpoint, not a series
of new post-1.4.52 library revisions. The per-commit increment rule applies to
new development after that reconstruction checkpoint.
