#!/usr/bin/env python

import glob
import os
import sys

from lib.util import execute


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  coffeelint = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'coffeelint')
  if sys.platform in ['win32', 'cygwin']:
    coffeelint += '.cmd'
  settings = ['--quiet', '-f', os.path.join('script', 'coffeelint.json')]
  files = glob.glob('atom/browser/api/lib/*.coffee') + \
          glob.glob('atom/renderer/api/lib/*.coffee') + \
          glob.glob('atom/common/api/lib/*.coffee') + \
          glob.glob('atom/browser/atom/*.coffee')

  execute([coffeelint] + settings + files)

if __name__ == '__main__':
  sys.exit(main())
