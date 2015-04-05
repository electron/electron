#!/usr/bin/env python

import os
import subprocess
import sys


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

def update_atom_modules(dirname):
  with scoped_cwd(dirname):
    apm = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'apm')
    if sys.platform in ['win32', 'cygwin']:
      apm = os.path.join(SOURCE_ROOT, 'node_modules', 'atom-package-manager',
                         'bin', 'apm.cmd')
    execute_stdout([apm, 'install'])

def main():
  os.chdir(SOURCE_ROOT)
  update_atom_modules('spec')

  if sys.platform == 'darwin':
    atom_shell = os.path.join(SOURCE_ROOT, 'out', 'Debug', 'Atom.app',
                              'Contents', 'MacOS', 'Atom')
  elif sys.platform == 'win32':
    atom_shell = os.path.join(SOURCE_ROOT, 'out', 'Debug', 'atom.exe')
  else:
    atom_shell = os.path.join(SOURCE_ROOT, 'out', 'Debug', 'atom')

  subprocess.check_call([atom_shell, 'spec'] + sys.argv[1:])

if __name__ == '__main__':
  sys.exit(main())
