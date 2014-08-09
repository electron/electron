#!/usr/bin/env python

import os
import subprocess
import sys

from lib.config import DIST_ARCH


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  update_external_binaries()
  update_gyp()


def update_external_binaries():
  uf = os.path.join('script', 'update-external-binaries.py')
  subprocess.check_call([sys.executable, uf])


def update_gyp():
  gyp = os.path.join('vendor', 'brightray', 'vendor', 'gyp', 'gyp_main.py')
  python = sys.executable
  arch = DIST_ARCH
  if sys.platform == 'darwin':
    # Only have 64bit build on OS X.
    arch = 'x64'
  elif sys.platform in ['cygwin', 'win32']:
    # Only have 32bit build on Windows.
    arch = 'ia32'
    if sys.platform == 'cygwin':
      # Force using win32 python on cygwin.
      python = os.path.join('vendor', 'python_26', 'python.exe')

  ret = subprocess.call([python, gyp,
                         '-f', 'ninja', '--depth', '.', 'atom.gyp',
                         '-Icommon.gypi', '-Ivendor/brightray/brightray.gypi',
                         '-Dlinux_clang=0',  # Disable brightray's clang setting
                         '-Dtarget_arch={0}'.format(arch),
                         '-Dlibrary=static_library'])
  if ret != 0:
    sys.exit(ret)


if __name__ == '__main__':
  sys.exit(main())
