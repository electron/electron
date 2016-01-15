#!/usr/bin/env python

import glob
import os
import sys

from lib.util import execute


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  eslint = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'eslint')
  if sys.platform in ['win32', 'cygwin']:
    eslint += '.cmd'
  settings = ['--quiet', '--config', os.path.join('script', 'eslintrc.json')]

  execute([eslint] + settings + ['atom'])


if __name__ == '__main__':
  sys.exit(main())
