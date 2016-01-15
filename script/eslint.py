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
  settings = ['--quiet', '--config']

  sourceConfig = os.path.join('script', 'eslintrc-base.json')
  sourceFiles = ['atom']
  execute([eslint] + settings + [sourceConfig] + sourceFiles)

  specConfig = os.path.join('script', 'eslintrc-spec.json')
  specFiles = glob.glob('spec/*.js')
  execute([eslint] + settings + [specConfig] + specFiles)


if __name__ == '__main__':
  sys.exit(main())
