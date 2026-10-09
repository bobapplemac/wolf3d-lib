"""Developer-only packaging tests; Python is not a build dependency.

Run with CMAKE pointing to a CMake executable, or with cmake on PATH.
These fixtures configure real CMake generators without needing every compiler.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CMAKE = os.environ.get('CMAKE', shutil.which('cmake') or 'cmake')


class Naming(unittest.TestCase):
    def configure(self, expected, backend='', **settings):
        defaults = dict(CMAKE_SYSTEM_NAME='Linux', CMAKE_SYSTEM_PROCESSOR='x86_64',
                        CMAKE_SIZEOF_VOID_P='8', CMAKE_C_COMPILER_ID='GNU',
                        CMAKE_C_COMPILER_VERSION='14.2.1', WG_LINUX_LIBC='glibc',
                        WIN32='FALSE', MSVC='FALSE', MINGW='FALSE',
                        WG_STATIC_MSVC_RUNTIME='ON', WG_STATIC_GNU_RUNTIME='ON',
                        WG_DEFAULT_OPL_DRIVER='silent', WG_DEFAULT_SAMPLE_RATE='22050',
                        WG_ENABLE_OPL_SILENT='ON', CMAKE_BUILD_TYPE='Release')
        defaults.update(settings)
        with tempfile.TemporaryDirectory(prefix='wolf dist ') as temp:
            source = Path(temp)
            lines = ['cmake_minimum_required(VERSION 3.5)', 'project(naming NONE)']
            lines += ['set(%s "%s")' % pair for pair in defaults.items()]
            lines += ['include("%s")' % (ROOT / 'cmake/WGDistribution.cmake').as_posix(),
                      'wg_distribution(PACKAGE %s 1.4.73 "%s" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/dist")'
                      % ('wolf3d-portable' if backend else 'wolf3d-lib', backend)]
            (source / 'CMakeLists.txt').write_text('\n'.join(lines))
            run = subprocess.run([CMAKE, '-S', str(source), '-B', str(source / 'build')],
                                 capture_output=True, text=True)
            log = run.stdout + run.stderr
            if expected is None:
                self.assertNotEqual(run.returncode, 0, log)
                return
            self.assertEqual(run.returncode, 0, log)
            config = defaults['CMAKE_BUILD_TYPE']
            path = (source / 'build' / ('PACKAGE-%s.path' % config)).read_text().strip()
            self.assertEqual(path, 'dist/' + expected)
            info = (source / 'build' / ('PACKAGE-%s-BUILD-INFO.txt' % config)).read_text()
            self.assertIn('Preferred sample rate: 22050 Hz', info)
            self.assertIn('Default OPL driver: silent', info)
            self.assertNotIn('22050', path)
            self.assertNotIn('silent', path)

    def test_msvc_toolsets(self):
        for version, toolset, label, platform in [
                (1200, '', '6.0', 'winxp'), (1300, '', '7.0', 'winxp'),
                (1310, '', '7.1', 'winxp'), (1400, '', 'v80', 'winxp'),
                (1500, '90', 'v90', 'winxp'), (1600, '100', 'v100', 'winxp'),
                (1700, '110', 'v110', 'win7'), (1800, '120', 'v120', 'win7'),
                (1900, '140', 'v140', 'win7'), (1916, '141', 'v141', 'win7'),
                (1929, '142', 'v142', 'win10'), (1930, '143', 'v143', 'win10'),
                (1950, '145', 'v145', 'win10')]:
            with self.subTest(label=label):
                self.configure('wolf3d-portable_1.4.73_%s_x86_gdi_msvc-%s' % (platform, label),
                               backend='gdi', WIN32='TRUE', MSVC='TRUE', MSVC_VERSION=version,
                               MSVC_TOOLSET_VERSION=toolset, CMAKE_SIZEOF_VOID_P=4)

    def test_xp_toolset_and_sdl_profiles(self):
        self.configure('wolf3d-portable_1.4.73_winxp_x64_gdi_msvc-v140',
                       backend='gdi', WIN32='TRUE', MSVC='TRUE', MSVC_VERSION=1900,
                       MSVC_TOOLSET_VERSION=140, CMAKE_VS_PLATFORM_TOOLSET='v140_xp')
        self.configure(None, backend='sdl3', WIN32='TRUE', MSVC='TRUE', MSVC_VERSION=1900,
                       MSVC_TOOLSET_VERSION=140, CMAKE_VS_PLATFORM_TOOLSET='v140_xp')
        self.configure('wolf3d-portable_1.4.73_win7_x64_sdl3_llvm-mingw14-msvcrt',
                       backend='sdl3', WIN32='TRUE', MINGW='TRUE', CMAKE_C_COMPILER_ID='Clang',
                       WG_DIST_PLATFORM='win7', WG_DIST_CRT='msvcrt')

    def test_compilers_and_runtime(self):
        for compiler, label in [('GNU', 'mingw-gcc14'), ('Clang', 'llvm-mingw14')]:
            for crt in ['msvcrt', 'ucrt']:
                for static, suffix in [('ON', ''), ('OFF', '_dynamic-gcc-runtime')]:
                    self.configure('wolf3d-lib_1.4.73_win7_x64_%s-%s%s' % (label, crt, suffix),
                                   WIN32='TRUE', MINGW='TRUE', CMAKE_C_COMPILER_ID=compiler,
                                   WG_DIST_CRT=crt, WG_DIST_PLATFORM='win7', WG_STATIC_GNU_RUNTIME=static)

    def test_linux_and_configurations(self):
        for libc in ['glibc', 'musl']:
            for compiler, label in [('GNU', 'gcc14'), ('Clang', 'clang14')]:
                for config, suffix in [('Release', ''), ('Debug', '_debug'),
                                       ('RelWithDebInfo', '_relwithdebinfo'), ('MinSizeRel', '_minsizerel')]:
                    self.configure('wolf3d-portable_1.4.73_linux-%s_x64_kms-fbdev_%s%s' % (libc, label, suffix),
                                   backend='kms-fbdev', WG_LINUX_LIBC=libc,
                                   CMAKE_C_COMPILER_ID=compiler, CMAKE_BUILD_TYPE=config)

    def test_architecture_and_invalid_vocabulary(self):
        self.configure('wolf3d-lib_1.4.73_linux-glibc_arm64_gcc14', CMAKE_SYSTEM_PROCESSOR='aarch64')
        self.configure(None, CMAKE_SYSTEM_PROCESSOR='ppc64')
        self.configure(None, CMAKE_BUILD_TYPE='Custom')
        self.configure('wolf3d-lib_1.4.73_win7_x64_msvc-v120_debug_dynamic-crt',
                       WIN32='TRUE', MSVC='TRUE', MSVC_VERSION=1800, MSVC_TOOLSET_VERSION=120,
                       CMAKE_BUILD_TYPE='Debug', WG_STATIC_MSVC_RUNTIME='OFF')
        self.configure(None, WG_DIST_PLATFORM='windows')
        self.configure(None, backend='win32')
        self.configure(None, WIN32='TRUE', MINGW='TRUE', WG_DIST_CRT='unknown')

    def test_build_info_preserves_new_options(self):
        with tempfile.TemporaryDirectory(prefix='wolf info ') as temp:
            root = Path(temp)
            info = root / 'BUILD-INFO.txt'
            cache = root / 'CMakeCache.txt'
            settings = root / 'target-settings.txt'
            info.write_text('Build summary\n')
            cache.write_text('FUTURE_BUILD_OPTION:BOOL=ON\nCUSTOM_LINK_FLAGS:STRING=-example\n')
            settings.write_text('wolf3d.COMPILE_DEFINITIONS: NEW_DEFINE=1\n')
            run = subprocess.run([CMAKE, '-DINFO=' + str(info), '-DROOT=' + str(root),
                                  '-DENGINE=' + str(root), '-DCACHE=' + str(cache),
                                  '-DSETTINGS=' + str(settings), '-P',
                                  str(ROOT / 'cmake/WGStampBuildInfo.cmake')],
                                 capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            result = info.read_text()
            self.assertIn(cache.read_text(), result)
            self.assertIn(settings.read_text(), result)
            self.assertIn('unknown', result)


if __name__ == '__main__':
    unittest.main()
