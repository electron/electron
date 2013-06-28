#!/usr/bin/env python

import subprocess
import sys

from lib.util import *


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))
NODE_VERSION = 'v0.10.12'


def main():
  os.chdir(SOURCE_ROOT)

  update_frameworks_and_node(NODE_VERSION)
  update_gyp()


def update_frameworks_and_node(version):
  if sys.platform == 'darwin':
    uf = os.path.join(SOURCE_ROOT, 'script', 'update-frameworks.py')
    subprocess.check_call([sys.executable, uf])

  un = os.path.join(SOURCE_ROOT, 'script', 'update-node.py')
  subprocess.check_call([sys.executable, un, '--version', version])


def update_gyp():
  gyp = os.path.join(SOURCE_ROOT, 'vendor', 'gyp', 'gyp')
  subprocess.check_call([sys.executable, gyp,
                         '-f', 'ninja', '--depth', '.', 'atom.gyp',
                         '-Icommon.gypi', '-Ivendor/brightray/brightray.gypi',
                         '-Dtarget_arch=ia32', '-Dlibrary=static_library'])


if __name__ == '__main__':
  sys.exit(main())
