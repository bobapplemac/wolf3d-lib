"""Exercise legacy IDE batch dispatch with a recording CMake stand-in (Windows)."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]

@unittest.skipUnless(os.name == 'nt', 'Windows CMD and Windows PowerShell required')
class LegacyIDETests(unittest.TestCase):
    def test_legacy_ide_actions(self):
        (ROOT / 'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='legacy-ide-', dir=ROOT / 'build') as temporary:
            base = Path(temporary)
            fixture = base / 'source with spaces'
            binary = base / 'bin'
            binary.mkdir()
            dispatcher = fixture / 'scripts/windows/legacy/build.cmd'
            dispatcher.parent.mkdir(parents=True)
            shutil.copyfile(ROOT / 'scripts/windows/legacy/build.cmd', dispatcher)
            source = base / 'FakeCMake.cs'
            source.write_text('class FakeCMake { static int Main(string[] args) { '
                              'System.Console.WriteLine("CMAKE:" + string.Join("|", args)); return 0; } }')
            compile_script = base / 'compile.ps1'
            compile_script.write_text('param($Source, $Output)\n'
                'Add-Type -Path $Source -OutputAssembly $Output -OutputType ConsoleApplication\n')
            compiled = subprocess.run(['powershell', '-NoProfile', '-File', str(compile_script),
                                       str(source), str(binary / 'cmake.exe')], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            env = dict(os.environ, PATH=str(binary) + os.pathsep + os.environ['PATH'])
            for band in ('vc6', 'vs2002', 'vs2003', 'vs2005'):
                helper = fixture / 'ide/visual-studio' / band / 'build.cmd'
                helper.parent.mkdir(parents=True)
                shutil.copyfile(ROOT / 'ide/visual-studio' / band / 'build.cmd', helper)
                expected = 'legacy-%s-all-nuked-static' % band
                output = fixture / 'build' / expected
                output.mkdir(parents=True)
                (output / 'CMakeCache.txt').write_text('fixture')
                for action in ('build', 'rebuild', 'clean', 'publish'):
                    with self.subTest(band=band, action=action):
                        run = subprocess.run(['cmd', '/d', '/c', str(helper), 'Release', action],
                                             env=env, capture_output=True, text=True)
                        log = run.stdout + run.stderr
                        self.assertEqual(run.returncode, 0, log)
                        self.assertNotIn('Unsupported driver set', log)
                        if action != 'clean':
                            for driver in ('NUKED', 'DBOPL', 'SILENT'):
                                self.assertIn('-DWG_ENABLE_OPL_%s=ON' % driver, log)
                            self.assertIn(expected, log)
                        self.assertEqual('--target|clean' in log, action in ('clean', 'rebuild'))
                        if action == 'publish':
                            self.assertRegex(log, r'--target\|(?:win32|library)_release')
                for project in (ROOT / 'ide/visual-studio' / band).glob('*.vcproj'):
                    for tool in ET.parse(project).iter('Tool'):
                        if tool.attrib.get('Name') == 'VCNMakeTool':
                            self.assertIn(expected, tool.attrib['Output'])

if __name__ == '__main__':
    unittest.main()
