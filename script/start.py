#!/usr/bin/env python

import os
import subprocess
import sys

from lib.util import electron_gyp


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']


def main():
  os.chdir(SOURCE_ROOT)

  config = 'D'
  if '-R' in sys.argv:
    config = 'R'

  if sys.platform == 'darwin':
    electron = os.path.join(SOURCE_ROOT, 'out', config,
                              '{0}.app'.format(PRODUCT_NAME), 'Contents',
                              'MacOS', PRODUCT_NAME)
  elif sys.platform == 'win32':
    electron = os.path.join(SOURCE_ROOT, 'out', config,
                              '{0}.exe'.format(PROJECT_NAME))
  else:
    electron = os.path.join(SOURCE_ROOT, 'out', config, PROJECT_NAME)

  try:
    subprocess.check_call([electron] + sys.argv[1:])
  except KeyboardInterrupt:
    return -1


if __name__ == '__main__':
  sys.exit(main())
