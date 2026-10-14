"""Windows dispatch regressions: dry-run modern scripts and stub legacy CMake."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]

@unittest.skipUnless(os.name == 'nt', 'Windows dispatch test')
class WindowsAudioChoices(unittest.TestCase):
    def test_legacy_validation_and_auto(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            scripts = root / 'scripts/windows/legacy'
            scripts.mkdir(parents=True)
            (scripts / 'build.cmd').write_bytes((ROOT / 'scripts/windows/legacy/build.cmd').read_bytes())
            (scripts / 'cmake-driver.cmd').write_text('@echo off\necho CMAKE %*\nexit /b 0\n')
            for drivers in ('all', 'dbopl', 'silent', 'nuked-dbopl'):
                result = subprocess.run(['cmd', '/c', str(scripts / 'build.cmd'), 'vc6', 'Release', drivers],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn('-DWG_ENABLE_OPL_SILENT=ON', result.stdout)
                self.assertIn('-DWG_DEFAULT_OPL_DRIVER=auto', result.stdout)
                self.assertNotIn('SAMPLE_RATE', result.stdout)
            for args in (['nonsense'], ['dbopl', 'nuked']):
                result = subprocess.run(['cmd', '/c', str(scripts / 'build.cmd'), 'vc6', 'Release', *args],
                                        capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn('CMAKE ', result.stdout)

    @unittest.skipUnless(Path('C:/msys64/ucrt64/bin/gcc.exe').exists(), 'MSYS2 UCRT64 required for dry-run detection')
    def test_modern_driver_flags(self):
        for drivers, adlib in (('all', 'OFF'), ('all-adlib', 'ON'), ('adlib', 'ON'), ('dbopl', 'OFF')):
            result = subprocess.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                                     str(ROOT / 'scripts/windows/invoke-build.ps1'), '-Compiler', 'mingw-ucrt64',
                                     '-Drivers', drivers, '-DryRun'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('-DWG_ENABLE_OPL_SILENT=ON', result.stdout)
            self.assertIn('-DWG_ENABLE_OPL_ADLIB=' + adlib, result.stdout)
            self.assertIn('-DWG_DEFAULT_OPL_DRIVER=auto', result.stdout)
            self.assertNotIn('SAMPLE_RATE=', result.stdout)
            if (ROOT / 'platforms/sdl3/WG_SDL3.c').exists():
                self.assertIn('-DW3P_ENABLE_INPOUT32=' + adlib, result.stdout)

if __name__ == '__main__':
    unittest.main()
