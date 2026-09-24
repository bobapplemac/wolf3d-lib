# Development log

Graphical captures are generated from the user's external Wolfenstein 3D data
and remain under the Git-ignored `out/` directory. They are never distributed
with the source repository. Each entry records a deterministic indexed-frame
hash so the milestone remains verifiable even when the screenshot is absent.

## 2026-09-23: Initial E1M1 walls and doors

- Commit: `a3f6c09`
- Scope: fixed-point ray traversal, original wall textures, closed door plane,
  ceiling/floor colors; no actors, first-person weapon, or status bar.
- 320x200 indexed-frame FNV-1a: `52a9cf2dd9dcab66`
- Identical result with the supplied WL1 and WL6 data sets.

![Initial E1M1 wall and door view](../out/initial-play-view.png)

## 2026-09-23: Original static status bar

- Scope: `STATUSBARPIC`, initial BJ face, floor, score, lives, health, ammo,
  empty key slots, and pistol icon, ported into `WL_AGENT.c`.
- 320x200 indexed-frame FNV-1a: `b2f43cae26b556f5`
- Identical result with the supplied WL1 and WL6 data sets.

![Initial E1M1 view with status bar](../out/initial-hud-view.png)

Regenerate the latest capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --dump-frame out\initial-hud-view.ppm
```

The PPM is the authoritative output. The PNG shown above is a convenience copy
created locally for visual inspection.

## 2026-09-23: First-person ready pistol

- Scope: the original compiled pistol sprite rendered with a portable
  `SimpleScaleShape` translation at the original 161-unit weapon scale.
- 320x200 indexed-frame FNV-1a, including the HUD: `ab0c1a3f48fece62`
- Identical result with the supplied WL1 and WL6 data sets.

![Initial E1M1 view with pistol and status bar](../out/initial-weapon-view.png)

## 2026-09-23: Static objects and wall occlusion

- Scope: original plane-1 static-object scan, `TransformTile` projection,
  ray-traversal `spotvis`, far-to-near ordering, and per-column `ScaleShape`
  wall occlusion.
- The normal initial frame remains `ab0c1a3f48fece62`: its closed door correctly
  hides the corridor scenery.
- The diagnostic below uses the same E1M1 starting pose with all doors in their
  original fully-open state so ceiling lights and floor objects are visible.
- 320x200 indexed-frame FNV-1a, including weapon and HUD: `9e72f0e803766954`
- Identical result with the supplied WL1 and WL6 data sets.

![E1M1 open-door static-object rendering](../out/open-door-static-objects.png)

Regenerate the diagnostic capture with:

```text
wolf3dgeneric-headless --data "C:\path\to\Wolf3D data" --play-view --open-doors --dump-frame out\open-door-static-objects.ppm
```
