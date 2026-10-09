"""Read-only maintainer checks for generated, mirrored and unreferenced assets.

Text references are review leads, not proof of reachability or safe deletion.
Python and an optional peer checkout are needed only for this explicit audit.
"""
import argparse
import fnmatch
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def normalized(path):
    return path.read_bytes().replace(b'\r\n', b'\n')


def compare_mirrors(root, peer, paths):
    errors = []
    for name in paths:
        left, right = root / name, peer / name
        if not left.is_file() or not right.is_file():
            errors.append('Missing mirrored file: ' + name)
        elif normalized(left) != normalized(right):
            errors.append('Mirror drift: ' + name)
    return errors


def identity(root):
    if (root / 'platforms/WG_ENGINE_COMPAT.h').is_file():
        return 'wolf3d-portable'
    if (root / 'include/WOLF3D.h').is_file():
        return 'wolf3d-lib'
    raise ValueError('Not a recognized wolf3d repository: ' + str(root))


def reference_report(root, inventory, verbose=False):
    names = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
                                     '--exclude-standard'], cwd=root, text=True).splitlines()
    texts = {}
    for name in sorted(set(names)):
        path = root / name
        if name.startswith(('third_party/', 'lib/', 'docs/archive/', 'docs/proposals/')) or not path.is_file():
            continue
        if name in ('tools/maintenance-inventory.json', 'docs/maintenance.md'):
            continue  # The inventory must not count as a caller of itself.
        raw = path.read_bytes()
        if b'\x00' not in raw:
            texts[name] = raw.decode('utf-8', errors='replace').replace('\\', '/')
    candidates = [name for name in texts if name.startswith(('scripts/', 'tools/', 'tests/', 'packaging/', 'cmake/'))
                  and not name.endswith('.md')]
    unreviewed = []
    reviewed = []
    for name in candidates:
        references = [other for other, text in texts.items()
                      if other != name and Path(name).name in text]
        if references:
            if verbose:
                print(name + " <- " + ", ".join(references))
            continue
        reason = inventory['manual_checks'].get(name)
        for pattern, explanation in inventory['dynamic_references'].items():
            if fnmatch.fnmatchcase(name, pattern):
                reason = explanation
        (reviewed if reason else unreviewed).append(name)
    print(f'Reference scan: {len(candidates)} first-party support assets; '
          f'{len(reviewed)} reviewed manual/dynamic cases; {len(unreviewed)} review leads.')
    for name in reviewed:
        print('  Retain (manual/dynamic): ' + name)
    for name in unreviewed:
        print('  REVIEW (no text references): ' + name)
    return unreviewed


def check_dos_source_coverage(root, outputs):
    recipe = (root / 'scripts/linux/openwatcom/build-dos.sh').read_text()
    required = set(re.findall(r'\$root/(platforms/[A-Za-z0-9_/]+\.c)', recipe))
    descriptors = '\n'.join((root / name).read_text().replace('\\', '/')
                            for name in outputs if name.endswith('.tgt'))
    present = set(re.findall(r'platforms/[A-Za-z0-9_/]+\.c', descriptors))
    return ['DOS IDE omits release source: ' + name for name in sorted(required - present)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--peer', type=Path, help='other repository checkout; never fetches or updates it')
    parser.add_argument('--references', action='store_true', help='list textual referrers for every support asset')
    args = parser.parse_args()
    data = json.loads((ROOT / 'tools/maintenance-inventory.json').read_text())
    product = identity(ROOT)
    local = data['repositories'][product]
    errors = []
    for name in [local['generator']] + local['generated_outputs'] + list(local['manual_checks']):
        if not (ROOT / name).is_file():
            errors.append('Missing inventoried asset: ' + name)
    for entry in local.get('external_generated', []):
        for key in ('output', 'generator'):
            if not (ROOT / entry[key]).is_file():
                errors.append('Missing external-generated asset: ' + entry[key])
        print('SKIP regeneration of ' + entry['output'] + ': ' + entry['inputs'])
    run = subprocess.run([sys.executable, str(ROOT / local['generator']), '--check'], cwd=ROOT)
    if run.returncode:
        errors.append('Regenerate the Open Watcom descriptors with ' + local['generator'])
    if product == 'wolf3d-portable' and all((ROOT / p).is_file() for p in local['generated_outputs']):
        errors.extend(check_dos_source_coverage(ROOT, local['generated_outputs']))
    if args.peer:
        if identity(args.peer) == product:
            errors.append('--peer must be the other project, not another copy of this one')
        else:
            errors.extend(compare_mirrors(ROOT, args.peer, data['mirrors']['paths']))
            print('Compared ' + str(len(data['mirrors']['paths'])) + ' registered mirror files (LF/CRLF normalized).')
    else:
        print('SKIP mirror comparison: pass --peer PATH for the other repository.')
    leads = reference_report(ROOT, local, args.references)
    if leads:
        errors.append('Review the unreferenced leads; document retained manual/dynamic uses in the inventory.')
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print('Local maintenance checks passed.' if not args.peer else 'Maintenance checks passed, including mirrors.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
