#!/usr/bin/env python

import os
import subprocess
import sys


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))
WINDOWS_NODE_PATHs = os.environ['PATH'].split(os.pathsep) + [
  'C:/Program Files (x86)/nodejs',
  'C:/Program Files/nodejs',
]
NIX_NODE_PATHs = os.environ['PATH'].split(os.pathsep) + [
  '/usr/local/bin',
  '/usr/bin',
  '/bin',
]

def main():
  input_file = sys.argv[1]
  output_dir = os.path.dirname(sys.argv[2])

  coffee = os.path.join(SOURCE_ROOT, 'node_modules', 'coffee-script', 'bin',
                        'coffee')

  node = 'node'
  if sys.platform in ['win32', 'cygwin']:
    node = find_node(WINDOWS_NODE_PATHs, 'node.exe')
  else:
    node = find_node(NIX_NODE_PATHs, 'node')

  if not node:
    print 'Node.js not found in PATH for building electron'
    return 1

  subprocess.check_call(['node', coffee, '-c', '-o', output_dir, input_file],
                        executable=node)


def find_node(paths, target):
  for path in paths:
    full_path = os.path.join(path, target)
    if os.path.exists(full_path):
      return full_path
  return None


if __name__ == '__main__':
  sys.exit(main())
