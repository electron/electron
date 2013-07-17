#!/usr/bin/env python

import os
import subprocess
import sys


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  if sys.platform == 'darwin':
    atom_shell = os.path.join(SOURCE_ROOT, 'out', 'Debug', 'Atom.app',
                              'Contents', 'MacOS', 'Atom')
  else:
    atom_shell = os.path.join(SOURCE_ROOT, 'out', 'Debug', 'atom.exe')

  subprocess.check_call([atom_shell, os.path.join(SOURCE_ROOT, 'spec')])


if __name__ == '__main__':
  sys.exit(main())
