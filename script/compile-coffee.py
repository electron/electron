#!/usr/bin/env python

import os
import subprocess
import sys

from lib.util import *


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))


def main():
  input_file = sys.argv[1]
  output_dir = os.path.dirname(sys.argv[2])

  node = get_node_path()
  coffee = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'coffee')
  subprocess.check_call([node, coffee, '-c', '-o', output_dir, input_file])


if __name__ == '__main__':
  sys.exit(main())
