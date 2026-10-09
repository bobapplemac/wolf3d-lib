"""Exercise legacy IDE batch dispatch with a recording CMake stand-in (Windows)."""
import os
import itertools
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]

class ProjectItemsTests(unittest.TestCase):
    def test_native_cpp_projects_use_explicit_existing_files(self):
        namespace = '{http://schemas.microsoft.com/developer/msbuild/2003}'
        for project in (ROOT / 'ide/visual-studio').glob('*/*.vcxproj'):
            with self.subTest(project=project):
                tree = ET.parse(project)
                entries = []
                for kind in ('ClCompile', 'ClInclude', 'None'):
                    for item in tree.iter(namespace + kind):
                        name = item.attrib.get('Include')
                        if name is None:
                            continue
                        for unsupported in ('*', '?', ';', '$(', '@('):
                            self.assertNotIn(unsupported, name)
                        resolved = (project.parent / name.replace('\\', '/')).resolve()
                        self.assertTrue(resolved.is_file(), str(resolved))
                        self.assertNotIn((kind, resolved), entries)
                        entries.append((kind, resolved))
                self.assertTrue(entries)

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
            shutil.copyfile(ROOT / 'scripts/windows/legacy/cmake-driver.cmd', dispatcher.with_name('cmake-driver.cmd'))
            source = base / 'FakeCMake.cs'
            source.write_text('class FakeCMake { static int Main(string[] args) { '
                              'System.Console.WriteLine("CMAKE_EXEC:" + System.Reflection.Assembly.GetExecutingAssembly().Location); '
                              'System.Console.WriteLine("CMAKE:" + string.Join("|", args)); return 0; } }')
            compile_script = base / 'compile.ps1'
            compile_script.write_text('param($Source, $Output)\n'
                'Add-Type -Path $Source -OutputAssembly $Output -OutputType ConsoleApplication\n')
            compiled = subprocess.run(['powershell', '-NoProfile', '-File', str(compile_script),
                                       str(source), str(binary / 'cmake.exe')], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            installed = base / 'Program Files/CMake/bin'
            installed.mkdir(parents=True)
            shutil.copyfile(binary / 'cmake.exe', installed / 'cmake.exe')
            for band in ('vc6', 'vs2002', 'vs2003', 'vs2005'):
                helper = fixture / 'ide/visual-studio' / band / 'build.cmd'
                helper.parent.mkdir(parents=True)
                shutil.copyfile(ROOT / 'ide/visual-studio' / band / 'build.cmd', helper)
                expected = 'legacy-%s-all-nuked-static' % band
                output = fixture / 'build' / expected
                output.mkdir(parents=True)
                (output / 'CMakeCache.txt').write_text('fixture')
                for action, mode in itertools.product(('build', 'rebuild', 'clean', 'publish'),
                                                       ('path', 'cache', 'installed', 'override')):
                    env = {k: v for k, v in os.environ.items()
                           if k.lower() not in ('path', 'programfiles', 'programfiles(x86)',
                                                'programw6432', 'wolf3d_legacy_cmake')}
                    env['PATH'] = str(Path(os.environ['SystemRoot']) / 'System32')
                    env['ProgramFiles'] = str(base / 'Program Files')
                    cache = 'fixture'
                    if mode == 'path': env['PATH'] = str(binary) + os.pathsep + env['PATH']
                    if mode == 'cache': cache = 'CMAKE_COMMAND:INTERNAL=' + str(binary / 'cmake.exe')
                    if mode == 'override': env['WOLF3D_LEGACY_CMAKE'] = str(binary / 'cmake.exe')
                    (output / 'CMakeCache.txt').write_text(cache + '\n')
                    with self.subTest(band=band, action=action, mode=mode):
                        run = subprocess.run(['cmd', '/d', '/c', str(helper), 'Release', action],
                                             env=env, capture_output=True, text=True)
                        log = run.stdout + run.stderr
                        self.assertEqual(run.returncode, 0, log)
                        self.assertNotIn('Unsupported driver set', log)
                        selected_binary = installed if mode == 'installed' else binary
                        self.assertIn('CMAKE_EXEC:' + str(selected_binary / 'cmake.exe'), log)
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
