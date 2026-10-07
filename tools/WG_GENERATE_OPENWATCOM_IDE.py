#!/usr/bin/env python3
"""Regenerate Open Watcom IDE descriptors from the canonical source list."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IDE = ROOT / "ide" / "open-watcom"


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

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(("\r\n".join(lines) + "\r\n").encode("ascii"))


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
    path.write_bytes(("\r\n".join(lines) + "\r\n").encode("ascii"))


def main():
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

    write_target(IDE / "engine" / "wolf3d-lib.tgt", "WOLF3D.LIB",
                 engine_relative)
    write_target(IDE / "nuked-opl3" / "nuked-opl3.tgt", "NUKEDOPL.LIB",
                 nuked_relative)
    write_project(IDE / "wolf3d-lib.wpj",
                  ["engine\\wolf3d-lib.tgt",
                   "nuked-opl3\\nuked-opl3.tgt"])


if __name__ == "__main__":
    main()
