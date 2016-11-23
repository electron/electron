#!/usr/bin/env python

import glob
import os
import subprocess
import sys

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  pylint = os.path.join(SOURCE_ROOT, 'vendor', 'depot_tools', 'pylint.py')
  settings = ['--rcfile=vendor/depot_tools/pylintrc']
  pys = glob.glob('script/*.py')
  env = os.environ.copy()
  env['PYTHONPATH'] = 'script'
  subprocess.check_call([sys.executable, pylint] + settings + pys, env=env)


if __name__ == '__main__':
  sys.exit(main())
