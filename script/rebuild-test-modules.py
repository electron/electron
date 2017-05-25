#!/usr/bin/env python

import argparse
import os
import shutil
import subprocess
import sys

from lib.config import PLATFORM, enable_verbose_mode, get_target_arch
from lib.util import execute_stdout, get_electron_version, safe_mkdir, \
                     update_node_modules, update_electron_modules


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()
  config = args.configuration

  if args.verbose:
    enable_verbose_mode()

  spec_modules = os.path.join(SOURCE_ROOT, 'spec', 'node_modules')
  out_dir = os.path.join(SOURCE_ROOT, 'out', config)
  version = get_electron_version()
  node_dir = os.path.join(out_dir, 'node-{0}'.format(version))

  # Create node headers
  script_path = os.path.join(SOURCE_ROOT, 'script', 'create-node-headers.py')
  execute_stdout([sys.executable, script_path, '--version', version,
                  '--directory', out_dir])

  if PLATFORM == 'win32':
    lib_dir = os.path.join(node_dir, 'Release')
    safe_mkdir(lib_dir)
    iojs_lib = os.path.join(lib_dir, 'iojs.lib')
    atom_lib = os.path.join(out_dir, 'node.dll.lib')
    shutil.copy2(atom_lib, iojs_lib)

  # Native modules can only be compiled against release builds on Windows
  if config == 'R' or PLATFORM != 'win32':
    update_electron_modules(os.path.dirname(spec_modules), get_target_arch(),
                            node_dir)
  else:
    update_node_modules(os.path.dirname(spec_modules))


def parse_args():
  parser = argparse.ArgumentParser(description='Rebuild native test modules')
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      help='Prints the output of the subprocesses')
  parser.add_argument('-c', '--configuration',
                      help='Build configuration to rebuild modules against',
                      default='D',
                      required=False)
  return parser.parse_args()


if __name__ == '__main__':
  sys.exit(main())
