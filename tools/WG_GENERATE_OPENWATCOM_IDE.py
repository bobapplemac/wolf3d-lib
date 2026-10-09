#!/usr/bin/env python3
"""Regenerate Open Watcom IDE descriptors from the canonical source list."""

import argparse
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IDE = ROOT / "ide" / "open-watcom"

CHECK = False
DRIFT = []


def write_output(path, lines):
    expected = ("\n".join(lines) + "\n").encode("ascii")
    if CHECK:
        actual = path.read_bytes().replace(b"\r\n", b"\n") if path.exists() else None
        if actual != expected:
            DRIFT.append(str(path.relative_to(IDE)))
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(expected)


def emit_string(lines, object_id, class_name, value):
    lines.extend((str(object_id), class_name, str(len(value)), value))


def write_target(path, output_name, sources):
    lines = [
        "39", "targetIdent", "0", "MProject", "1", "MComponent", "0"
    ]
    emit_string(lines, 2, "WString", "LIB")
    emit_string(lines, 3, "WString", "d_2sn")
    lines.extend(("1", "0", "1", "4", "MCommand", "0", "5", "MCommand",
                  "0", "6", "MItem"))
    lines.extend((str(len(output_name)), output_name))
    emit_string(lines, 7, "WString", "LIB")
    lines.extend(("8", "WVList", "0", "9", "WVList", "0", "-1", "1",
                  "1", "0", "10", "WPickList", str(len(sources) + 1)))

    all_items = ["*.c"] + sources
    wildcard_id = 11
    for index, source in enumerate(all_items):
        item_id = wildcard_id + index * 4
        lines.extend((str(item_id), "MItem", str(len(source)), source))
        emit_string(lines, item_id + 1, "WString", "COBJ")
        lines.extend((str(item_id + 2), "WVList", "0", str(item_id + 3),
                      "WVList", "0", "-1" if index == 0 else str(wildcard_id),
                      "1", "1", "0"))

    write_output(path, lines)


def write_project(path, targets):
    lines = ["39", "projectIdent", "0", "VpeMain", "1", "WRect", "0", "0",
             "7680", "10240", "2", "MProject", "3", "MCommand", "0", "4",
             "MCommand", "0", str(len(targets))]
    for object_id, target in enumerate(targets, 5):
        emit_string(lines, object_id, "WFileName", target)
    list_id = 5 + len(targets)
    lines.extend((str(list_id), "WVList", str(len(targets))))
    first_component = list_id + 1
    for index, target in enumerate(targets):
        component_id = first_component + index * 3
        lines.extend((str(component_id), "VComponent", str(component_id + 1),
                      "WRect", str(index * 3900), "120", "3750", "3880", "0",
                      "0"))
        emit_string(lines, component_id + 2, "WFileName", target)
        lines.extend(("0", "-1"))
    lines.append(str(first_component))
    write_output(path, lines)


def main():
    global CHECK, IDE
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='fail on drift without writing files')
    parser.add_argument('--output-dir', type=Path, default=IDE, help='alternative descriptor directory')
    args = parser.parse_args()
    CHECK, IDE = args.check, args.output_dir
    DRIFT.clear()
    core = [line.strip() for line in
            (ROOT / "cmake" / "WGCoreSources.txt").read_text().splitlines()
            if line.strip()]
    engine_sources = core + [
        "src/WG_OPL.c", "src/WG_OPL_NUKED.c", "src/WG_OPL_DBOPL.c",
        "src/WG_OPL_SILENT.c", "src/WG_OPL_ADLIB.c",
        "third_party/DBOPL/dbopl.c",
    ]
    engine_relative = ["..\\..\\..\\" + source.replace("/", "\\")
                       for source in engine_sources]
    nuked_relative = ["..\\..\\..\\third_party\\Nuked-OPL3\\opl3.c"]

    write_target(IDE / "engine" / "wolf3d-lib.tgt", "WOLF3D.lib",
                 engine_relative)
    write_target(IDE / "nuked-opl3" / "nuked-opl3.tgt", "NUKEDOPL.lib",
                 nuked_relative)
    write_project(IDE / "wolf3d-lib.wpj",
                  ["engine\\wolf3d-lib.tgt",
                   "nuked-opl3\\nuked-opl3.tgt"])

    if DRIFT:
        print('Generated descriptor drift: ' + ', '.join(DRIFT), file=sys.stderr)
        return 1
    if CHECK:
        print('Open Watcom descriptors match the generator.')
    return 0


if __name__ == "__main__":
    sys.exit(main())
