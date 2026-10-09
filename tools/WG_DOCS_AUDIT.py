"""Audit first-party Markdown navigation. Developer-only; no build dependency.

Checks local inline/reference links, Markdown anchors, table column counts and
coverage of docs/ in its index. External URLs and fenced code are not fetched.
Vendored third-party documentation is outside this repository's ownership.
"""
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]


def prose(text):
    lines = []
    fence = None
    for line in text.splitlines():
        marker = re.match(r'^\s*(`{3,}|~{3,})', line)
        if marker:
            if fence is None:
                fence = marker[1][0]
            elif marker[1][0] == fence:
                fence = None
            continue
        if fence is None:
            lines.append(line)
    return lines


def anchors(path):
    found = set()
    counts = {}
    for line in prose(path.read_text(encoding='utf-8-sig')):
        heading = re.match(r'^#{1,6}\s+(.+?)\s*#*$', line)
        if heading:
            text = re.sub(r'\[([^]]+)\]\([^)]+\)', r'\1', heading[1]).lower()
            slug = re.sub(r'[^\w -]', '', text).replace(' ', '-')
            count = counts.get(slug, 0)
            counts[slug] = count + 1
            found.add(slug + ('-' + str(count) if count else ''))
        found.update(re.findall(r'(?:id|name)=["\']([^"\']+)["\']', line))
    return found


def main():
    names = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
                                     '--exclude-standard', '--', '*.md'], cwd=ROOT,
                                    text=True).splitlines()
    files = [ROOT / n for n in sorted(set(names))
             if not n.startswith(('third_party/', 'lib/')) and (ROOT / n).is_file()]
    errors = []
    indexed = set()
    count = 0
    for path in files:
        expected_columns = None
        for number, line in enumerate(prose(path.read_text(encoding='utf-8-sig')), 1):
            links = re.findall(r'\[[^]]*\]\(<?([^\s)>]+)>?(?:\s+"[^"]*")?\)', line)
            reference = re.match(r'^\s*\[[^]]+\]:\s*<?([^\s>]+)>?', line)
            if reference:
                links.append(reference[1])
            for url in links:
                if re.match(r'^[a-zA-Z][a-zA-Z0-9+.-]*:|^//', url):
                    continue
                relative, _, anchor = unquote(url).partition('#')
                target = (path.parent / relative).resolve() if relative else path
                count += 1
                if not target.exists():
                    errors.append(f'{path.relative_to(ROOT)}: missing {url}')
                elif anchor and target.suffix.lower() == '.md' and anchor not in anchors(target):
                    errors.append(f'{path.relative_to(ROOT)}: missing anchor {url}')
                if path == ROOT / 'docs/README.md':
                    indexed.add(target)
            stripped = re.sub(r'`[^`]*`', 'CODE', line.strip())
            if stripped.startswith('|') and stripped.endswith('|'):
                columns = len(re.split(r'(?<!\\)\|', stripped)) - 2
                if expected_columns is not None and columns != expected_columns:
                    errors.append(f'{path.relative_to(ROOT)}: inconsistent table columns: {line}')
                expected_columns = columns
            else:
                expected_columns = None
    for path in files:
        if path.is_relative_to(ROOT / 'docs') and path != ROOT / 'docs/README.md' and path.resolve() not in indexed:
            errors.append(f'docs/README.md: not indexed: {path.relative_to(ROOT)}')
    if errors:
        print('\n'.join(errors))
        return 1
    print(f'Documentation audit passed: {len(files)} files, {count} local links, complete docs index.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
