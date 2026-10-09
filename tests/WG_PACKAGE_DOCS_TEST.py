"""Developer-only package documentation tests; no Python build dependency."""
import itertools
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CMAKE = os.environ.get('CMAKE', shutil.which('cmake') or 'cmake')

class PackageDocs(unittest.TestCase):
    def fixture(self, directory, nuked, dbopl, sdl, dos):
        (directory / 'DOCS/LICENSES').mkdir(parents=True)
        (directory / 'README.TXT').write_text('Quick start.\n')
        (directory / 'DOCS/BUILD.TXT').write_text('All options: custom=preserved\n')
        (directory / 'DOCS/ENGINE.TXT').write_text('engine revision: exact\n')
        shutil.copyfile(ROOT / 'LICENSE', directory / 'DOCS/LICENSES/GPL-2.TXT')
        for flag, name in [(nuked, 'LGPL-21'), (sdl, 'SDL3'), (dos, 'DOS32A')]:
            if flag:
                (directory / ('DOCS/LICENSES/' + name + '.TXT')).write_bytes(b'Unmodified license fixture.\n')
        if dbopl:
            (directory / 'DOCS/DBOPL.TXT').write_text('Exact DBOPL provenance fixture.\n')

    def test_selected_components_and_merged_metadata(self):
        for product in ['wolf3d-lib', 'wolf3d-portable']:
            for flags in itertools.product([False, True], repeat=4):
                with self.subTest(product=product, flags=flags), tempfile.TemporaryDirectory(prefix='wolf docs ') as temp:
                    outputs = []
                    methods = ['cmake'] + (['shell'] if os.name != 'nt' else [])
                    for method in methods:
                        directory = Path(temp) / method
                        self.fixture(directory, *flags)
                        before = {p.name: p.read_bytes() for p in (directory / 'DOCS/LICENSES').iterdir()}
                        if method == 'cmake':
                            command = [CMAKE, '-DDIST=' + directory.as_posix(), '-DROOT=' + ROOT.as_posix(), '-DPRODUCT=' + product, '-P', str(ROOT / 'cmake/WGPackageDocs.cmake')]
                        else:
                            command = ['sh', str(ROOT / 'scripts/package-docs.sh'), str(ROOT), str(directory), product]
                        subprocess.run(command, check=True, capture_output=True)
                        self.assertEqual(sorted(p.name for p in directory.iterdir()), ['DOCS', 'README.TXT'])
                        self.assertEqual(sorted(p.name for p in (directory / 'DOCS').iterdir()), ['BUILD.TXT', 'LICENSES', 'NOTICES.TXT'])
                        for p in directory.rglob('*'):
                            self.assertRegex(p.name, r'^[A-Z0-9-]{1,8}(\.[A-Z0-9-]{1,3})?$')
                        self.assertEqual(before, {p.name: p.read_bytes() for p in (directory / 'DOCS/LICENSES').iterdir()})
                        notices = (directory / 'DOCS/NOTICES.TXT').read_text()
                        for enabled, marker in zip(flags, ['Nuked-OPL3:', 'DBOPL C port:', 'SDL3:', 'DOS/32A:']):
                            self.assertEqual(marker in notices, enabled)
                        self.assertEqual('wolf3d-portable:' in notices, product == 'wolf3d-portable')
                        build = (directory / 'DOCS/BUILD.TXT').read_text()
                        self.assertIn('custom=preserved', build)
                        self.assertIn('engine revision: exact', build)
                        self.assertEqual('Exact DBOPL provenance fixture.' in notices, flags[1])
                        outputs.append(notices.strip())
                    self.assertTrue(all(out == outputs[0] for out in outputs))

    @unittest.skipUnless(os.environ.get('WG_PACKAGE_DIRS'), 'set WG_PACKAGE_DIRS to validate built distributions')
    def test_built_packages(self):
        for item in os.environ['WG_PACKAGE_DIRS'].split(os.pathsep):
            directory = Path(item)
            with self.subTest(package=item):
                if directory.name.startswith('wolf3d-portable_'):
                    if '_dos32_' in directory.name or '_win9x_' in directory.name:
                        executable = 'WOLF3D.EXE'
                    elif '_win' in directory.name:
                        executable = 'wolf3d.exe'
                    else:
                        executable = 'wolf3d'
                    self.assertTrue((directory / executable).is_file())
                    self.assertFalse((directory / 'wolf3d-sdl3').exists())
                    self.assertFalse((directory / 'wolf3d-sdl3.exe').exists())
                    if '_linux-musl_' in directory.name:
                        self.assertTrue((directory / 'bin/wolf3d').is_file())
                self.assertTrue((directory / 'README.TXT').is_file())
                self.assertTrue((directory / 'DOCS/LICENSES/GPL-2.TXT').is_file())
                self.assertEqual(sorted(p.name for p in (directory / 'DOCS').iterdir()), ['BUILD.TXT', 'LICENSES', 'NOTICES.TXT'])
                for p in directory.iterdir():
                    if p.suffix.lower() == '.txt':
                        self.assertEqual(p.name, 'README.TXT')
                    self.assertNotIn(p.name.lower(), ['licenses'])
                for p in (directory / 'DOCS').rglob('*'):
                    self.assertRegex(p.name, r'^[A-Z0-9-]{1,8}(\.[A-Z0-9-]{1,3})?$')
                build = (directory / 'DOCS/BUILD.TXT').read_text()
                self.assertTrue('CMakeCache.txt' in build or 'Build recipe:' in build)
                self.assertIn('Source ', build)
                self.assertIn('wolf3d-lib:', (directory / 'DOCS/NOTICES.TXT').read_text())

    def test_incomplete_package_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            result = subprocess.run([CMAKE, '-DDIST=' + temp, '-DROOT=' + ROOT.as_posix(), '-P', str(ROOT / 'cmake/WGPackageDocs.cmake')], capture_output=True)
            self.assertNotEqual(result.returncode, 0)

if __name__ == '__main__':
    unittest.main()
