"""Maintainer audit regressions; no compilers or external data required."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('maintenance', ROOT / 'tools/WG_MAINTENANCE_AUDIT.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class MaintenanceTests(unittest.TestCase):
    def test_mirror_comparison(self):
        with tempfile.TemporaryDirectory() as temp:
            a, b = Path(temp) / 'a', Path(temp) / 'b'
            a.mkdir(); b.mkdir()
            (a / 'helper').write_bytes(b'original\n')
            (b / 'helper').write_bytes(b'original\r\n')
            self.assertEqual(audit.compare_mirrors(a, b, ['helper']), [])
            (b / 'helper').write_bytes(b'changed\n')
            self.assertIn('Mirror drift', audit.compare_mirrors(a, b, ['helper'])[0])
            self.assertIn('Missing', audit.compare_mirrors(a, b, ['missing'])[0])

    def test_generator_check_never_rewrites(self):
        generator = 'W3P_GENERATE_OPENWATCOM_IDE.py' if audit.identity(ROOT) == 'wolf3d-portable' else 'WG_GENERATE_OPENWATCOM_IDE.py'
        with tempfile.TemporaryDirectory(prefix='wolf maintenance ') as temp:
            output = Path(temp) / 'output'
            command = [sys.executable, str(ROOT / 'tools' / generator), '--output-dir', str(output)]
            missing = subprocess.run(command + ['--check'], capture_output=True)
            self.assertNotEqual(missing.returncode, 0)
            self.assertFalse(output.exists())
            subprocess.run(command, check=True, capture_output=True)
            before = {p: (p.read_bytes(), p.stat().st_mtime_ns) for p in output.rglob('*') if p.is_file()}
            subprocess.run(command + ['--check'], check=True, capture_output=True)
            self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_mtime_ns) for p in before})
            target = next(output.rglob('*.tgt'))
            target.write_bytes(b'intentionally edited\n')
            bad = subprocess.run(command + ['--check'], capture_output=True)
            self.assertNotEqual(bad.returncode, 0)
            self.assertEqual(target.read_bytes(), b'intentionally edited\n')

    def test_dos_coverage_detects_missing_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            recipe = root / 'scripts/linux/openwatcom/build-dos.sh'
            recipe.parent.mkdir(parents=True)
            recipe.write_text('compile "$root/platforms/WG_HELP.c"\n')
            target = root / 'game.tgt'
            target.write_text('other.c\n')
            self.assertTrue(audit.check_dos_source_coverage(root, ['game.tgt']))
            target.write_text('../../../platforms/WG_HELP.c\n')
            self.assertEqual(audit.check_dos_source_coverage(root, ['game.tgt']), [])


if __name__ == '__main__':
    unittest.main()
