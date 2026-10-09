"""Local-only integration tests for the shared build Git preflight."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BASH = shutil.which('bash') if os.name != 'nt' else r'C:\Program Files\Git\bin\bash.exe'
GIT = shutil.which('git')

class PreflightTests(unittest.TestCase):
    def setUp(self):
        (ROOT / 'build').mkdir(exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(prefix='git-preflight-', dir=ROOT / 'build')
        self.base = Path(self.tmp.name)
        self.env = dict(os.environ, GIT_ALLOW_PROTOCOL='file', GIT_TERMINAL_PROMPT='0',
                        WOLF3D_GIT_INTERACTIVE='1', WOLF3D_GIT_CHECK='1')
        self.seed = self.base / 'seed'
        self.remote = self.base / 'remote.git'
        self.clone = self.base / 'checkout with spaces'
        self.git(self.base, 'init', '--bare', str(self.remote))
        self.git(self.base, 'init', '-b', 'main', str(self.seed))
        (self.seed / 'content').write_text('initial\n')
        self.commit(self.seed)
        self.git(self.seed, 'remote', 'add', 'origin', str(self.remote))
        self.git(self.seed, 'push', '-u', 'origin', 'main')
        self.git(self.base, 'clone', '-b', 'main', str(self.remote), str(self.clone))
        self.initial = self.git(self.clone, 'rev-parse', 'HEAD').strip()

    def tearDown(self):
        # TemporaryDirectory is confined to this repository's ignored build tree.
        self.tmp.cleanup()

    def git(self, cwd, *args):
        p = subprocess.run([GIT, '-c', 'user.name=Build Test', '-c', 'user.email=build-test@example.invalid',
                            '-C', str(cwd), *args], env=self.env, text=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEqual(p.returncode, 0, p.stderr)
        return p.stdout

    def commit(self, repo):
        self.git(repo, 'add', '.')
        self.git(repo, 'commit', '-m', 'fixture')

    def advance(self):
        (self.seed / 'new-file').write_text('upstream\n')
        self.commit(self.seed)
        self.git(self.seed, 'push')
        return self.git(self.seed, 'rev-parse', 'HEAD').strip()

    def run_check(self, answer='', **env):
        p = subprocess.run([BASH, str(ROOT / 'scripts/git-preflight.sh'), str(self.clone)],
                           input=answer, text=True, env=dict(self.env, **env),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        self.assertEqual(p.returncode, 0, p.stdout)
        return p.stdout

    def head(self):
        return self.git(self.clone, 'rev-parse', 'HEAD').strip()

    def test_decline_and_eof_preserve_head(self):
        self.advance()
        self.assertIn('declined', self.run_check('n\n'))
        self.assertEqual(self.head(), self.initial)
        self.run_check('')
        self.assertEqual(self.head(), self.initial)

    def test_confirm_fast_forward(self):
        target = self.advance()
        self.run_check('y\n')
        self.assertEqual(self.head(), target)

    def test_up_to_date(self):
        self.assertIn('up to date', self.run_check())

    def test_dirty_preserved(self):
        self.advance()
        (self.clone / 'content').write_text('local work\n')
        self.assertIn('Local changes', self.run_check('y\n'))
        self.assertEqual(self.head(), self.initial)
        self.assertEqual((self.clone / 'content').read_text(), 'local work\n')

    def test_ahead_and_diverged_preserved(self):
        (self.clone / 'local-file').write_text('local commit\n')
        self.commit(self.clone)
        local = self.head()
        self.assertIn('Local development history', self.run_check('y\n'))
        self.advance()
        self.assertIn('Local development history', self.run_check('y\n'))
        self.assertEqual(self.head(), local)

    def test_detached_and_no_upstream(self):
        self.git(self.clone, 'checkout', '--detach')
        self.assertIn('specific source version', self.run_check('y\n'))
        self.git(self.clone, 'checkout', '-b', 'local-branch')
        self.assertIn('No published update source', self.run_check('y\n'))

    def test_git_inspection_error_is_not_local_work(self):
        wrapper = self.base / 'fail-status.sh'
        wrapper.write_bytes(b'''#!/usr/bin/env bash
git() {
    if [ "$1" = status ]; then
        echo 'fatal: detected dubious ownership in repository' >&2
        return 128
    fi
    command git "$@"
}
export -f git
bash "$1" "$2"
''')
        run = subprocess.run([BASH, str(wrapper), str(ROOT / 'scripts/git-preflight.sh'), str(self.clone)],
                             env=self.env, text=True, input='y\n', capture_output=True)
        output = run.stdout + run.stderr
        self.assertNotEqual(run.returncode, 0, output)
        self.assertIn('Git could not inspect', output)
        self.assertNotIn('Local changes detected', output)
        self.assertEqual(self.head(), self.initial)

    def test_engine_minimum_version_diagnostic(self):
        module = ROOT / 'cmake/W3PCheckEngineVersion.cmake'
        cmake = os.environ.get('W3P_CMAKE') or shutil.which('cmake')
        if not module.exists() or not cmake:
            self.skipTest('portable CMake version check required')
        script = self.base / 'version.cmake'
        for version, succeeds in (('1.4.56', False), ('1.4.57', True), ('1.4.70', True)):
            with self.subTest(version=version):
                script.write_text('include("' + module.as_posix() + '")\nw3p_check_engine_version("' + version + '")\n')
                run = subprocess.run([cmake, '-P', str(script)], capture_output=True, text=True)
                self.assertEqual(run.returncode == 0, succeeds, run.stdout + run.stderr)
                if not succeeds:
                    self.assertIn('wolf3d_GetCommandLineHelp', run.stderr)
                    self.assertIn('too old', run.stderr)

    def test_offline(self):
        self.git(self.clone, 'remote', 'set-url', 'origin', str(self.base / 'absent.git'))
        self.assertIn('a newer version may be available', self.run_check('y\n'))
        self.assertEqual(self.head(), self.initial)

    def test_noninteractive_and_opt_out(self):
        self.advance()
        self.assertIn('Noninteractive', self.run_check('y\n', WOLF3D_GIT_INTERACTIVE='0'))
        self.assertEqual(self.head(), self.initial)
        self.assertEqual(self.run_check('y\n', WOLF3D_GIT_CHECK='0'), '')
        self.assertEqual(self.head(), self.initial)

    def dependency(self, tracking):
        dep = self.base / 'dependency'
        self.git(self.base, 'init', '-b', 'main', str(dep))
        (dep / 'source').write_text('old\n')
        (dep / 'include').mkdir()
        (dep / 'include/WOLF3D.h').write_text('#define WOLF3D_PLATFORM_API_VERSION 6U\n')
        self.commit(dep)
        args = ['submodule', 'add']
        if tracking:
            args += ['-b', 'main']
            (self.seed / 'platforms').mkdir()
            (self.seed / 'platforms/WG_ENGINE_COMPAT.h').write_text('#define W3P_ENGINE_API_VERSION 6U\n')
        self.git(self.seed, *args, str(dep), 'lib/wolf3d')
        self.commit(self.seed)
        self.git(self.seed, 'push')
        (dep / 'source').write_text('new\n')
        self.commit(dep)
        return self.git(dep, 'rev-parse', 'HEAD').strip()

    def test_pinned_dependency_stays_recorded(self):
        newest = self.dependency(False)
        self.run_check('y\ny\n')
        actual = self.git(self.clone / 'lib/wolf3d', 'rev-parse', 'HEAD').strip()
        self.assertNotEqual(actual, newest)
        self.assertEqual(self.git(self.clone, 'status', '--porcelain'), '')

    def test_engine_updates_with_one_confirmation(self):
        newest = self.dependency(True)
        self.assertIn('Engine updated', self.run_check('y\n'))
        self.assertEqual(self.git(self.clone / 'lib/wolf3d', 'rev-parse', 'HEAD').strip(), newest)
        self.assertIn('lib/wolf3d', self.git(self.clone, 'status', '--porcelain'))
        self.assertIn('engine is up to date', self.run_check())
        # A later engine bugfix needs no application commit.
        dep = self.base / 'dependency'
        (dep / 'source').write_text('another fix')
        self.commit(dep)
        target = self.git(dep, 'rev-parse', 'HEAD').strip()
        self.assertIn('declined', self.run_check('n\n'))
        self.assertEqual(self.git(self.clone / 'lib/wolf3d', 'rev-parse', 'HEAD').strip(), newest)
        self.assertIn('Engine updated', self.run_check('y\n'))
        self.assertEqual(self.git(self.clone / 'lib/wolf3d', 'rev-parse', 'HEAD').strip(), target)
        # A subsequent application update also accepts the managed engine selection.
        self.advance()
        self.assertIn('Engine updated', self.run_check('y\n'))
        self.assertEqual(self.git(self.clone / 'lib/wolf3d', 'rev-parse', 'HEAD').strip(), target)

    def test_incompatible_engine_keeps_existing(self):
        self.dependency(True)
        dep = self.base / 'dependency'
        (dep / 'include/WOLF3D.h').write_text('#define WOLF3D_PLATFORM_API_VERSION 7U\n')
        self.commit(dep)
        self.assertIn('different API', self.run_check('y\n'))
        self.assertEqual(self.git(self.clone, 'status', '--porcelain'), '')

    def test_custom_engine_selection_preserved(self):
        self.dependency(True)
        self.run_check('y\n')
        self.git(self.clone / 'lib/wolf3d', 'checkout', '--detach', 'HEAD~1')
        # Select a different clean engine version, not the managed one or the pin.
        (self.clone / 'lib/wolf3d/source').write_text('custom commit')
        self.commit(self.clone / 'lib/wolf3d')
        self.assertIn('Local changes', self.run_check('y\n'))

    def test_visual_studio_cache_does_not_block_updates(self):
        shutil.copyfile(ROOT / '.gitignore', self.seed / '.gitignore')
        self.commit(self.seed)
        self.git(self.seed, 'push')
        self.git(self.clone, 'pull', '--ff-only')
        generated = ['ide/visual-studio/vs2019/.vs/Solution/cache']
        generated += ['ide/visual-studio/vc6/project.' + suffix for suffix in ('ncb', 'opt', 'plg')]
        for band in ('vs2002', 'vs2003', 'vs2005'):
            generated += ['ide/visual-studio/' + band + '/project.ncb',
                          'ide/visual-studio/' + band + '/obj/Debug/BuildLog.htm']
        for band in ('vs2010', 'vs2012', 'vs2013'):
            generated += ['ide/visual-studio/' + band + '/project.' + suffix
                          for suffix in ('sdf', 'opensdf')]
        for name in generated:
            cache = self.clone / name
            cache.parent.mkdir(parents=True, exist_ok=True)
            cache.write_text('generated IDE data')
        target = self.advance()
        self.run_check('y\n')
        self.assertEqual(self.head(), target)
        for name in generated:
            self.assertEqual((self.clone / name).read_text(), 'generated IDE data')
        solution = self.clone / 'ide/visual-studio/vs2005/project.sln'
        solution.write_text('local solution')
        self.assertIn('Local changes', self.run_check('y\n'))

    def test_untracked_work_preserved(self):
        self.advance()
        (self.clone / 'my-notes').write_text('keep me')
        self.assertIn('Local changes', self.run_check('y\n'))
        self.assertEqual(self.head(), self.initial)

    def test_dependency_work_preserved(self):
        self.dependency(True)
        self.run_check('y\nn\n')
        source = self.clone / 'lib/wolf3d/source'
        source.write_text('my engine change')
        self.git(self.clone, 'config', 'submodule.lib/wolf3d.ignore', 'all')
        self.advance()
        current = self.head()
        self.assertIn('Local changes', self.run_check('y\ny\n'))
        self.assertEqual(self.head(), current)
        self.assertEqual(source.read_text(), 'my engine change')

    def test_compile_api_guard(self):
        header = ROOT / 'platforms/WG_ENGINE_COMPAT.h'
        compiler = shutil.which('cc') or shutil.which('gcc') or shutil.which('clang')
        if not header.exists() or not compiler:
            self.skipTest('portable API header and C compiler required')
        for version, succeeds in ((6, True), (5, False), (7, False)):
            with self.subTest(version=version):
                (self.base / 'WOLF3D.h').write_text('#define WOLF3D_PLATFORM_API_VERSION %dU\n' % version)
                result = subprocess.run([compiler, '-E', '-x', 'c', '-I', str(self.base),
                                         '-I', str(header.parent), '-'],
                                        input='#include "WG_ENGINE_COMPAT.h"\n', text=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                self.assertEqual(result.returncode == 0, succeeds, result.stderr)
                if not succeeds:
                    self.assertIn('Incompatible wolf3d-lib API', result.stderr)

    def test_root_frontends_forward_arguments(self):
        for name in ('build.sh', 'build.ps1', 'build.cmd',
                     'scripts/git-preflight.sh', 'scripts/windows/git-preflight.cmd'):
            destination = self.clone / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, destination)
        driver = self.clone / 'scripts/linux/invoke-build.sh'
        driver.parent.mkdir(parents=True, exist_ok=True)
        driver.write_bytes(b'#!/usr/bin/env bash\nprintf "FORWARDED:%s|%s" "$1" "$2"\n')
        driver.chmod(0o755)
        commands = [([BASH, str(self.clone / 'build.sh'), 'help', 'JOBS=2'], 'FORWARDED:help|JOBS=2')]
        if os.name == 'nt':
            driver = self.clone / 'scripts/windows/invoke-build.ps1'
            driver.write_text('Write-Output ("FORWARDED:" + ($args -join "|"))')
            commands.append((['powershell', '-NoProfile', '-File', str(self.clone / 'build.ps1'),
                              '-List', '-NonInteractive'], 'FORWARDED:-List|-NonInteractive'))
            driver = self.clone / 'scripts/windows/legacy/build.cmd'
            driver.parent.mkdir(parents=True, exist_ok=True)
            driver.write_text('@echo off\necho FORWARDED:%1:%2\nexit /b 0\n')
            commands.append((['cmd', '/d', '/c', str(self.clone / 'build.cmd'), 'vc6', 'debug'],
                             'FORWARDED:vc6:debug'))
        for command, expected in commands:
            with self.subTest(command=command):
                result = subprocess.run(command, text=True, env=self.env,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertIn('Local changes', result.stdout)
                self.assertIn(expected, result.stdout)

    def test_msys2_frontends_find_bash(self):
        msys_bin = Path(os.environ.get('MSYS2_ROOT', r'C:\msys64')) / 'usr/bin'
        if os.name != 'nt' or not (msys_bin / 'git.exe').exists():
            self.skipTest('MSYS2 installation required')
        self.env['PATH'] = str(msys_bin) + os.pathsep + self.env['PATH']
        self.test_root_frontends_forward_arguments()

    def test_missing_dependency_confirmation(self):
        self.dependency(False)
        self.git(self.clone, 'pull', '--ff-only')
        self.assertIn('declined', self.run_check('n\n'))
        self.assertTrue(self.git(self.clone, 'submodule', 'status').startswith('-'))
        self.run_check('y\n')
        self.assertTrue(self.git(self.clone, 'submodule', 'status').startswith(' '))

if __name__ == '__main__':
    unittest.main()
