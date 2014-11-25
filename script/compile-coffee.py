#!/usr/bin/env python

import os
import subprocess
import sys


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))


def main():
  input_file = sys.argv[1]
  output_dir = os.path.dirname(sys.argv[2])

  coffee = os.path.join(SOURCE_ROOT, 'node_modules', 'coffee-script', 'bin',
                        'coffee')
  if sys.platform in ['win32', 'cygwin']:
    node = find_node()
    if not node:
      print 'Node.js not found in PATH for building atom-shell'
      return 1
    subprocess.check_call(['node', coffee, '-c', '-o', output_dir, input_file],
                          executable=node)
  else:
    subprocess.check_call(['node', coffee, '-c', '-o', output_dir, input_file])


def find_node():
  PATHs = os.environ['PATH'].split(os.pathsep)
  for path in PATHs:
    full_path = os.path.join(path, 'node.exe')
    if os.path.exists(full_path):
      return full_path
  return None


if __name__ == '__main__':
  sys.exit(main())
