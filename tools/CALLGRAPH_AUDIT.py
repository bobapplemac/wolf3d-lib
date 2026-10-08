#!/usr/bin/env python3
"""Static source-parity call-graph audit for original Wolf3D and wolf3dgeneric.

This intentionally uses a tolerant lexer rather than a C compiler: the original
sources contain Borland extensions, K&R declarations, conditional variants and
callback tables which no single modern compilation database represents.
"""

import argparse
import fnmatch
import json
import re
import sys
from collections import defaultdict, deque
from pathlib import Path


PROFILES = {
    "wl1": {"UPLOAD"},
    "wl6": set(),
    "sdm": {"SPEAR", "SPEARDEMO"},
    "sod": {"SPEAR"},
}
CONTROL_WORDS = {"if", "for", "while", "switch", "return", "sizeof"}
ORIGINAL_ROOTS = {"main", "WolfMain", "DemoLoop", "GameLoop"}
PORTABLE_ROOTS = {
    "wolf3dgeneric_Create", "wolf3dgeneric_Run", "wolf3dgeneric_Shutdown",
    "wolf3dgeneric_Tick", "wolf3dgeneric_GetVersion",
    "wolf3dgeneric_SetPlatform",
}


def strip_comments_and_literals(text):
    pattern = re.compile(
        r"//[^\n]*|/\*.*?\*/|\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'",
        re.S,
    )
    return pattern.sub(lambda m: "\n" * m.group(0).count("\n"), text)


def eval_if_expression(expression, defines):
    expression = re.sub(
        r"defined\s*\(\s*([A-Za-z_]\w*)\s*\)",
        lambda m: "1" if m.group(1) in defines else "0", expression)
    expression = re.sub(
        r"defined\s+([A-Za-z_]\w*)",
        lambda m: "1" if m.group(1) in defines else "0", expression)
    expression = re.sub(
        r"\b[A-Za-z_]\w*\b",
        lambda m: "1" if m.group(0) in defines else "0", expression)
    expression = expression.replace("&&", " and ").replace("||", " or ")
    expression = re.sub(r"!(?!=)", " not ", expression)
    if not re.fullmatch(r"[\d\s()<>!=&|+\-*/.andornot]+", expression):
        return True
    try:
        return bool(eval(expression, {"__builtins__": {}}, {}))
    except (SyntaxError, TypeError, ZeroDivisionError):
        return True


def preprocess(text, initial_defines):
    defines = set(initial_defines)
    output = []
    stack = []
    active = True
    directive = re.compile(r"^\s*#\s*(\w+)(.*)$")
    for line in text.splitlines(True):
        match = directive.match(line)
        if not match:
            output.append(line if active else "\n" if line.endswith("\n") else "")
            continue
        command, argument = match.group(1), match.group(2).strip()
        if command in ("ifdef", "ifndef", "if"):
            parent = active
            if command == "ifdef":
                condition = argument in defines
            elif command == "ifndef":
                condition = argument not in defines
            else:
                condition = eval_if_expression(argument, defines)
            stack.append([parent, condition, condition])
            active = parent and condition
        elif command == "elif" and stack:
            parent, _condition, taken = stack[-1]
            condition = not taken and eval_if_expression(argument, defines)
            stack[-1][1] = condition
            stack[-1][2] = taken or condition
            active = parent and condition
        elif command == "else" and stack:
            parent, _condition, taken = stack[-1]
            condition = not taken
            stack[-1][1] = condition
            stack[-1][2] = True
            active = parent and condition
        elif command == "endif" and stack:
            parent, _condition, _taken = stack.pop()
            active = parent
        elif command == "define" and active:
            name = argument.split("(", 1)[0].split(None, 1)[0] if argument else ""
            if name:
                defines.add(name)
        elif command == "undef" and active:
            defines.discard(argument.split(None, 1)[0])
        output.append("\n" if line.endswith("\n") else "")
    return "".join(output)


def matching_left(text, right, opening, closing):
    depth = 1
    for index in range(right - 1, -1, -1):
        if text[index] == closing:
            depth += 1
        elif text[index] == opening:
            depth -= 1
            if depth == 0:
                return index
    return -1


def matching_right(text, left, opening, closing):
    depth = 1
    for index in range(left + 1, len(text)):
        if text[index] == opening:
            depth += 1
        elif text[index] == closing:
            depth -= 1
            if depth == 0:
                return index
    return -1


def extract_functions(text, source):
    clean = strip_comments_and_literals(text)
    functions = {}
    depth = 0
    last_top_boundary = 0
    index = 0
    while index < len(clean):
        char = clean[index]
        if char == "{" and depth == 0:
            prefix = clean[max(last_top_boundary, index - 4096):index]
            closes = [m.start() for m in re.finditer(r"\)", prefix)]
            found = None
            for close_local in reversed(closes):
                close = max(last_top_boundary, index - 4096) + close_local
                open_paren = matching_left(clean, close, "(", ")")
                if open_paren < 0:
                    continue
                name_match = re.search(r"([A-Za-z_]\w*)\s*$", clean[:open_paren])
                if not name_match or name_match.group(1) in CONTROL_WORDS:
                    continue
                name = name_match.group(1)
                between = clean[close + 1:index]
                if "=" in between or re.search(r"\btypedef\b", between):
                    continue
                header_start = max(clean.rfind(";", last_top_boundary, name_match.start()),
                                   clean.rfind("}", last_top_boundary, name_match.start())) + 1
                header = clean[header_start:index]
                if re.search(r"\b(?:if|for|while|switch)\s*$", clean[header_start:name_match.start()]):
                    continue
                found = (name, name_match.start(), header)
                break
            body_end = matching_right(clean, index, "{", "}")
            if found and body_end >= 0:
                name, name_pos, _header = found
                line = clean.count("\n", 0, name_pos) + 1
                functions[name] = {
                    "name": name,
                    "source": source,
                    "line": line,
                    "body": clean[index + 1:body_end],
                }
                index = body_end
                last_top_boundary = body_end + 1
            else:
                depth += 1
        elif char == "{" :
            depth += 1
        elif char == "}":
            depth = max(0, depth - 1)
            if depth == 0:
                last_top_boundary = index + 1
        elif char == ";" and depth == 0:
            last_top_boundary = index + 1
        index += 1
    return functions, clean


def extract_state_tables(clean, source):
    states = {}
    pattern = re.compile(
        r"\bstatetype\s+([A-Za-z_]\w*)\s*=\s*\{([^{};]*)\}\s*;", re.S)
    for match in pattern.finditer(clean):
        fields = [field.strip() for field in match.group(2).split(",")]
        if len(fields) < 6:
            continue
        states[match.group(1)] = {
            "name": match.group(1), "source": source,
            "line": clean.count("\n", 0, match.start()) + 1,
            "rotate": fields[0], "shape": fields[1], "tics": fields[2],
            "think": fields[3], "action": fields[4],
            "next": fields[5].lstrip("&"),
        }
    return states


def identifiers(text):
    return set(re.findall(r"\b[A-Za-z_]\w*\b", text))


def scan_tree(root, patterns, defines):
    functions = {}
    states = {}
    texts = []
    files = []
    for pattern in patterns:
        files.extend(root.glob(pattern))
    for path in sorted(set(files)):
        try:
            raw = path.read_text(encoding="latin-1")
        except OSError:
            continue
        filtered = preprocess(raw, defines)
        relative = path.relative_to(root).as_posix()
        found, clean = extract_functions(filtered, relative)
        functions.update(found)
        states.update(extract_state_tables(clean, relative))
        texts.append(clean)
    names = set(functions)
    state_names = set(states)
    edges = defaultdict(set)
    for name, info in functions.items():
        refs = identifiers(info["body"])
        edges[name].update(refs & names)
        edges[name].update("state:" + ref for ref in refs & state_names)
    for state, info in states.items():
        node = "state:" + state
        for field in ("think", "action"):
            target = info[field]
            if target in names:
                edges[node].add(target)
        if info["next"] in state_names:
            edges[node].add("state:" + info["next"])
    return functions, states, edges, "\n".join(texts)


def reachable(edges, roots):
    seen = set()
    queue = deque(root for root in roots if root in edges or any(root in v for v in edges.values()))
    while queue:
        node = queue.popleft()
        if node in seen:
            continue
        seen.add(node)
        queue.extend(edges.get(node, ()))
    return seen


def canonical(name):
    value = name.lower()
    for prefix in ("wolf3dgeneric_", "wl_", "wg_", "id_"):
        if value.startswith(prefix):
            value = value[len(prefix):]
            break
    return re.sub(r"[^a-z0-9]", "", value)


def shortest_distance(edges, start, target, maximum=6):
    queue = deque([(start, 0)])
    seen = set()
    while queue:
        node, distance = queue.popleft()
        if node == target:
            return distance
        if node in seen or distance >= maximum:
            continue
        seen.add(node)
        queue.extend((child, distance + 1) for child in edges.get(node, ()))
    return None


def load_overrides(path):
    if not path or not path.exists():
        return {"mappings": {}, "classifications": []}
    return json.loads(path.read_text(encoding="utf-8"))


def classify(name, source, overrides):
    for rule in overrides.get("classifications", []):
        if fnmatch.fnmatch(name, rule.get("function", "*")) and fnmatch.fnmatch(
                source, rule.get("source", "*")):
            return rule.get("status", "documented"), rule.get("reason", "")
    return "unclassified", ""


def edge_exception(caller, callee, profile, overrides):
    for rule in overrides.get("edge_exceptions", []):
        profiles = rule.get("profiles")
        if (fnmatch.fnmatch(caller, rule.get("caller", "*"))
                and fnmatch.fnmatch(callee, rule.get("callee", "*"))
                and (not profiles or profile in profiles)):
            return rule.get("reason", "Reviewed structural translation.")
    return None


def make_mapping(original_functions, portable_functions, overrides):
    by_canonical = defaultdict(list)
    for name in portable_functions:
        by_canonical[canonical(name)].append(name)
    explicit = overrides.get("mappings", {})
    mapping = {}
    ambiguous = {}
    for name in original_functions:
        candidates = []
        target = explicit.get(name)
        if isinstance(target, str):
            candidates = [target] if target in portable_functions else []
        elif isinstance(target, list):
            candidates = [item for item in target if item in portable_functions]
        if not candidates:
            candidates = by_canonical.get(canonical(name), [])
        if len(candidates) == 1:
            mapping[name] = candidates[0]
        elif len(candidates) > 1:
            ambiguous[name] = candidates
    return mapping, ambiguous


def markdown_report(results, original_label):
    lines = [
        "# Original-source call-graph parity audit", "",
        "Generated by `tools/CALLGRAPH_AUDIT.py`. The report is a static-analysis",
        "lead generator, not proof of behavioral equivalence. Indirect hardware and",
        "assembly boundaries still require manual review and runtime tests.", "",
        f"Original source: `{original_label}`", "",
        "## Summary", "",
        "| Profile | Original functions | Original states | Chocolate functions | Mapped functions | Reviewed edge translations | Unmapped state callbacks | Bridge-confirmed missing | Bridge-confirmed path gaps |",
        "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |",
    ]
    for profile, data in results.items():
        lines.append(
            f"| {profile.upper()} | {data['function_count']} | {data['state_count']} | "
            f"{data['bridge_function_count']} | {data['mapped_count']} | "
            f"{len(data['reviewed_edges'])} | "
            f"{sum(1 for x in data['missing'] if x['state_callback'])} | "
            f"{sum(1 for x in data['missing'] if x['bridge_confirmed'])} | "
            f"{sum(1 for x in data['edge_gaps'] if x['bridge_confirmed'])} |")
    lines.extend(["", "## Chocolate-corroborated audit leads", ""])
    for profile, data in results.items():
        lines.extend([f"### {profile.upper()}", ""])
        confirmed_gaps = [x for x in data["edge_gaps"] if x["bridge_confirmed"]]
        confirmed_missing = [x for x in data["missing"] if x["bridge_confirmed"]]
        state_missing = [x for x in data["missing"] if x["state_callback"]]
        if not confirmed_gaps and not confirmed_missing:
            lines.append("No unclassified discrepancies.")
        if state_missing:
            lines.extend(["Original state-machine callbacks without a portable mapping or classification (highest-priority review queue):", ""])
            for item in state_missing:
                bridge = "yes" if item["bridge_confirmed"] else "not statically reached"
                lines.append(
                    f"- `{item['name']}` ({item['source']}:{item['line']}; "
                    f"Chocolate corroboration: {bridge}).")
        if confirmed_gaps:
            lines.extend(["Call paths present in both original and Chocolate but absent from the portable graph:", ""])
            for gap in confirmed_gaps[:80]:
                lines.append(
                    f"- `{gap['caller']}` → `{gap['callee']}` maps to "
                    f"`{gap['portable_caller']}` → `{gap['portable_callee']}`.")
        if confirmed_missing:
            lines.extend(["", "Routines reachable in both original and Chocolate without a portable mapping or classification:", ""])
            for item in confirmed_missing[:80]:
                lines.append(
                    f"- `{item['name']}` ({item['source']}:{item['line']}; "
                    f"incoming edges: {item['incoming']}).")
        lines.append("")
    lines.extend([
        "## Interpretation rules", "",
        "- A mapped edge is accepted when the portable callee is reachable from the",
        "  mapped caller within six calls; structural helper extraction is therefore",
        "  allowed without hiding disconnected implementations.",
        "- Actor `statetype` think/action/next-state entries are represented as graph",
        "  nodes and edges rather than ignored as data.",
        "- Classifications and renamed-function mappings live in",
        "  `tools/callgraph-overrides.json` and require a written reason.",
        "- Missing items are audit leads. They are not automatically bugs.", "",
        "- Chocolate Wolfenstein is corroborating evidence only. The DOS source",
        "  remains authoritative whenever the two differ.", "",
    ])
    return "\n".join(lines)


def write_dot(path, title, functions, states, edges, reachable_nodes):
    def quote(value):
        return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'

    lines = ["digraph callgraph {", f"  label={quote(title)};", "  labelloc=t;"]
    all_nodes = set(functions) | {"state:" + name for name in states}
    for node in sorted(all_nodes):
        if node.startswith("state:"):
            label = node[6:]
            shape = "box"
        else:
            info = functions[node]
            label = f"{node}\\n{info['source']}:{info['line']}"
            shape = "ellipse"
        color = "black" if node in reachable_nodes else "gray65"
        lines.append(
            f"  {quote(node)} [label={quote(label)}, shape={shape}, color={quote(color)}];")
    for caller in sorted(edges):
        for callee in sorted(edges[caller]):
            if caller in all_nodes and callee in all_nodes:
                lines.append(f"  {quote(caller)} -> {quote(callee)};")
    lines.append("}")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--original", required=True, type=Path,
                        help="Path to the original WOLFSRC directory")
    parser.add_argument("--portable", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--bridge", type=Path,
                        help="Path to Chocolate-Wolfenstein-3D sources")
    parser.add_argument("--overrides", type=Path,
                        default=Path(__file__).with_name("callgraph-overrides.json"))
    parser.add_argument("--report", type=Path)
    parser.add_argument("--json", type=Path)
    parser.add_argument("--dot-dir", type=Path,
                        help="Write Graphviz DOT graphs for all three trees")
    parser.add_argument("--fail-on-gaps", action="store_true")
    args = parser.parse_args()
    if not args.original.is_dir():
        parser.error("--original must name the original WOLFSRC directory")

    overrides = load_overrides(args.overrides)
    portable_functions, portable_states, portable_edges, _ = scan_tree(
        args.portable, ["src/*.c"], set())
    portable_reachable = reachable(portable_edges, PORTABLE_ROOTS)
    if args.dot_dir:
        write_dot(args.dot_dir / "wolf3dgeneric.dot", "wolf3dgeneric",
                  portable_functions, portable_states, portable_edges,
                  portable_reachable)
    results = {}
    any_gaps = False
    for profile, defines in PROFILES.items():
        original_functions, original_states, original_edges, _ = scan_tree(
            args.original, ["WL_*.C", "ID_*.C"], defines)
        original_reachable = reachable(original_edges, ORIGINAL_ROOTS)
        if args.bridge:
            bridge_functions, bridge_states, bridge_edges, _ = scan_tree(
                args.bridge, ["*.cpp", "*.c"], defines)
            bridge_reachable = reachable(bridge_edges, ORIGINAL_ROOTS)
            bridge_mapping, _ = make_mapping(
                original_functions, bridge_functions,
                {"mappings": {}, "classifications": []})
        else:
            bridge_functions, bridge_states, bridge_edges = {}, {}, defaultdict(set)
            bridge_reachable, bridge_mapping = set(), {}
        if args.dot_dir:
            write_dot(args.dot_dir / f"original-{profile}.dot",
                      f"Original Wolf3D {profile.upper()}", original_functions,
                      original_states, original_edges, original_reachable)
            if args.bridge:
                write_dot(args.dot_dir / f"chocolate-{profile}.dot",
                          f"Chocolate Wolfenstein {profile.upper()}",
                          bridge_functions, bridge_states, bridge_edges,
                          bridge_reachable)
        mapping, ambiguous = make_mapping(
            original_functions, portable_functions, overrides)
        incoming = defaultdict(int)
        for targets in original_edges.values():
            for target in targets:
                incoming[target] += 1
        state_callbacks = {
            states[field]
            for states in original_states.values()
            for field in ("think", "action")
            if states[field] in original_functions
        }
        missing = []
        for name in sorted(original_reachable):
            if name.startswith("state:") or name in mapping:
                continue
            info = original_functions.get(name)
            if not info:
                continue
            status, reason = classify(name, info["source"], overrides)
            if status == "unclassified":
                missing.append({
                    "name": name, "source": info["source"], "line": info["line"],
                    "incoming": incoming[name],
                    "state_callback": name in state_callbacks,
                    "bridge_confirmed": name in bridge_mapping
                                        and bridge_mapping[name] in bridge_reachable,
                })
        edge_gaps = []
        reviewed_edges = []
        for caller, targets in original_edges.items():
            if caller not in mapping or caller not in original_reachable:
                continue
            for callee in targets:
                if callee not in mapping:
                    continue
                exception_reason = edge_exception(caller, callee, profile,
                                                  overrides)
                if exception_reason is not None:
                    reviewed_edges.append({
                        "caller": caller, "callee": callee,
                        "reason": exception_reason,
                    })
                    continue
                distance = shortest_distance(
                    portable_edges, mapping[caller], mapping[callee])
                if distance is None:
                    bridge_distance = None
                    if caller in bridge_mapping and callee in bridge_mapping:
                        bridge_distance = shortest_distance(
                            bridge_edges, bridge_mapping[caller],
                            bridge_mapping[callee])
                    edge_gaps.append({
                        "caller": caller, "callee": callee,
                        "portable_caller": mapping[caller],
                        "portable_callee": mapping[callee],
                        "bridge_confirmed": bridge_distance is not None,
                    })
        results[profile] = {
            "function_count": len(original_functions),
            "state_count": len(original_states),
            "reachable_count": len(original_reachable),
            "bridge_function_count": len(bridge_functions),
            "bridge_state_count": len(bridge_states),
            "bridge_reachable_count": len(bridge_reachable),
            "mapped_count": len(mapping),
            "portable_function_count": len(portable_functions),
            "portable_state_count": len(portable_states),
            "portable_reachable_count": len(portable_reachable),
            "missing": missing,
            "edge_gaps": sorted(edge_gaps, key=lambda x: (x["caller"], x["callee"])),
            "reviewed_edges": sorted(reviewed_edges,
                                     key=lambda x: (x["caller"], x["callee"])),
            "ambiguous": ambiguous,
        }
        any_gaps |= bool(missing or edge_gaps)

    label = args.original.name
    report = markdown_report(results, label)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(report + "\n", encoding="utf-8")
    else:
        print(report)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    return 1 if args.fail_on_gaps and any_gaps else 0


if __name__ == "__main__":
    sys.exit(main())
