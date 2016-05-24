#!/usr/bin/env python

import os
import subprocess
import sys

from lib.util import atom_gyp, rm_rf


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

PROJECT_NAME = atom_gyp()['project_name%']
PRODUCT_NAME = atom_gyp()['product_name%']


def main():
  os.chdir(SOURCE_ROOT)

  config = 'D'
  if len(sys.argv) == 2 and sys.argv[1] == '-R':
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

  returncode = 0
  try:
    subprocess.check_call([electron, 'spec'] + sys.argv[1:])
  except subprocess.CalledProcessError as e:
    returncode = e.returncode

  if os.environ.has_key('OUTPUT_TO_FILE'):
    output_to_file = os.environ['OUTPUT_TO_FILE']
    with open(output_to_file, 'r') as f:
      print f.read()
    rm_rf(output_to_file)


  return returncode


if __name__ == '__main__':
  sys.exit(main())
