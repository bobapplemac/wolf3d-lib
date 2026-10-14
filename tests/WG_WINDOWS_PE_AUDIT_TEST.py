"""Linux maintainer regressions for the Windows 2000 package audit.
Run: python3 tests/WG_WINDOWS_PE_AUDIT_TEST.py
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BASE = "file format pei-i386\nMajorSubsystemVersion 4\nMinorSubsystemVersion 0\nDLL Name: KERNEL32.dll\nGetProcAddress\nDLL Name: msvcrt.dll\nmemcpy\n"

class Audit(unittest.TestCase):
    def check(self, extra='', fail=False, dll=False, base=BASE):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            package = root / 'package with spaces'
            package.mkdir()
            (package / 'wolf3d.exe').write_text(BASE if dll else base + extra)
            if dll:
                (package / 'wolf3d.dll').write_text(base + extra)
            objdump = root / 'objdump'
            objdump.write_text('#!/bin/sh\ncat "$3"\n')
            objdump.chmod(0o755)
            env = dict(os.environ, OBJDUMP=str(objdump))
            result = subprocess.run(['sh', str(ROOT / 'tools/WG_WINDOWS_PE_AUDIT.sh'),
                                     'mingw-win2k-x86', str(package)],
                                    env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode != 0, fail, result.stdout + result.stderr)

    def test_baseline(self):
        self.check()
        self.check(base=BASE.replace('MajorSubsystemVersion 4', 'MajorSubsystemVersion 5'))

    def test_exe_and_dll_imports(self):
        for symbol in ['GetRawInputData', 'RegisterRawInputDevices', 'EncodePointer',
                       'GetModuleHandleExW', 'GetTickCount64', 'InitializeCriticalSectionEx',
                       'DLL Name: ucrtbase.dll', 'DLL Name: api-ms-win-core-synch-l1-2-0.dll',
                       'DLL Name: libgcc_s_dw2-1.dll']:
            for dll in [False, True]:
                with self.subTest(symbol=symbol, dll=dll):
                    self.check(symbol + '\n', fail=True, dll=dll)

    def test_subsystem_and_architecture(self):
        self.check(base=BASE.replace('MajorSubsystemVersion 4', 'MajorSubsystemVersion 6'), fail=True)
        self.check(base=BASE.replace('MajorSubsystemVersion 4', 'MajorSubsystemVersion 5').replace('MinorSubsystemVersion 0', 'MinorSubsystemVersion 1'), fail=True)
        self.check(base=BASE.replace('pei-i386', 'pei-x86-64'), fail=True)
        self.check(base=BASE.replace('MinorSubsystemVersion 0', ''), fail=True)

if __name__ == '__main__':
    unittest.main()
