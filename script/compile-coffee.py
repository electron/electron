#!/usr/bin/env python

import os
import subprocess
import sys

from lib.util import *


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))


def main():
  input_file = sys.argv[1]
  output_dir = os.path.dirname(sys.argv[2])

  coffee = os.path.join(SOURCE_ROOT, 'node_modules', 'coffee-script', 'bin',
                        'coffee')
  if sys.platform in ['win32', 'cygwin']:
    subprocess.check_call(['node', coffee, '-c', '-o', output_dir, input_file],
                          executable='C:/Program Files/nodejs/node.exe')
  else:
    subprocess.check_call(['node', coffee, '-c', '-o', output_dir, input_file])

if __name__ == '__main__':
  sys.exit(main())
