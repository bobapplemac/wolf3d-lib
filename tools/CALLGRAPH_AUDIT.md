# Call-graph parity audit

`CALLGRAPH_AUDIT.py` compares three source trees:

1. id Software's original DOS source, separately preprocessed as WL1, WL6,
   SDM, and SOD;
2. Chocolate Wolfenstein 3D as a modern, fidelity-oriented corroborating
   implementation;
3. the wolf3dgeneric core library.

The original source remains authoritative. Chocolate raises the confidence of
an audit lead when it preserves the same call or state-table relationship; it
never overrides a difference found in the DOS source.

The analyzer is deliberately compiler-independent. It tolerates Borland/K&R
syntax and turns original `statetype` think, action, and next-state fields into
explicit graph edges. Renames and reviewed architectural substitutions are
recorded in `callgraph-overrides.json` with reasons.

Example:

```powershell
python tools\CALLGRAPH_AUDIT.py `
  --original "C:\path\to\wolf3d\WOLFSRC" `
  --bridge "C:\path\to\Chocolate-Wolfenstein-3D" `
  --report build\callgraph-audit.md `
  --json build\callgraph-audit.json `
  --dot-dir build\callgraphs
```

The Markdown report is meant for review. JSON retains all candidates for
filtering and future automation. DOT output can be rendered with Graphviz, but
Graphviz is not required to run the audit.

This is not a proof of behavioral identity. Timing, global side effects,
assembly routines, data-dependent dispatch, and consolidated state machines
still need targeted source review and deterministic runtime tests.
