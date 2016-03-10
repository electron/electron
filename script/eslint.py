#!/usr/bin/env python

import glob
import os
import sys

from lib.config import PLATFORM
from lib.util import execute


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  # Skip eslint on our Windows build machine for now.
  if PLATFORM == 'win32' and os.getenv('JANKY_SHA1'):
    return

  eslint = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'eslint')
  if sys.platform in ['win32', 'cygwin']:
    eslint += '.cmd'
  settings = ['--quiet', '--config']

  sourceConfig = os.path.join('script', 'eslintrc-base.json')
  sourceFiles = ['lib']
  execute([eslint] + settings + [sourceConfig] + sourceFiles)

  specConfig = os.path.join('script', 'eslintrc-spec.json')
  specFiles = glob.glob('spec/*.js')
  execute([eslint] + settings + [specConfig] + specFiles)


if __name__ == '__main__':
  sys.exit(main())
