#!/usr/bin/env python

import os
import subprocess
import sys


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  config = 'D'
  if len(sys.argv) == 2 and sys.argv[1] == '-R':
    config = 'R'

  if sys.platform == 'darwin':
    atom_shell = os.path.join(SOURCE_ROOT, 'out', config, 'Atom.app',
                              'Contents', 'MacOS', 'Atom')
  elif sys.platform == 'win32':
    atom_shell = os.path.join(SOURCE_ROOT, 'out', config, 'atom.exe')
  else:
    atom_shell = os.path.join(SOURCE_ROOT, 'out', config, 'atom')

  subprocess.check_call([atom_shell, 'spec'] + sys.argv[1:])


if __name__ == '__main__':
  sys.exit(main())
