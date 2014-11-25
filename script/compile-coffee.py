#!/usr/bin/env python

import os
import subprocess
import sys


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))
WINDOWS_NODE_PATHs = [
  'C:/Program Files/nodejs/node.exe',
  'C:/Program Files (x86)/nodejs/node.exe',
  'C:/nodejs/node.exe',
]


def main():
  input_file = sys.argv[1]
  output_dir = os.path.dirname(sys.argv[2])

  coffee = os.path.join(SOURCE_ROOT, 'node_modules', 'coffee-script', 'bin',
                        'coffee')
  if sys.platform in ['win32', 'cygwin']:
    node = find_node()
    if not node:
      print 'Node.js is required for building atom-shell at paths:\n' + \
            '\n'.join(WINDOWS_NODE_PATHs)
      return 1
    subprocess.check_call(['node', coffee, '-c', '-o', output_dir, input_file],
                          executable=node)
  else:
    subprocess.check_call(['node', coffee, '-c', '-o', output_dir, input_file])


def find_node():
  for path in WINDOWS_NODE_PATHs:
    if os.path.exists(path):
      return path
  return None


if __name__ == '__main__':
  sys.exit(main())
