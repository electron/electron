#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import enable_verbose_mode, get_target_arch
from lib.util import execute_stdout


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()
  if args.verbose:
    enable_verbose_mode()

  extra_update_args = []
  if args.disable_clang:
    extra_update_args += ['--disable_clang']
  if args.clang_dir:
    extra_update_args += ['--clang_dir', args.clang_dir]

  # ./script/bootstrap
  # ./script/update -t x64
  # ./script/build --no_shared_library -t x64
  # ./script/create-dist -c static_library -t x64 --no_zip
  script_dir = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor',
                            'libchromiumcontent', 'script')
  bootstrap = os.path.join(script_dir, 'bootstrap')
  update = os.path.join(script_dir, 'update')
  build = os.path.join(script_dir, 'build')
  create_dist = os.path.join(script_dir, 'create-dist')
  execute_stdout([sys.executable, bootstrap])
  execute_stdout([sys.executable, update, '-t', args.target_arch] +
                 extra_update_args)
  execute_stdout([sys.executable, build, '-R', '-t', args.target_arch])
  execute_stdout([sys.executable, create_dist, '-c', 'static_library',
                  '--no_zip', '-t', args.target_arch])


def parse_args():
  parser = argparse.ArgumentParser(description='Build libchromiumcontent')
  parser.add_argument('--target_arch',
                      help='Specify the arch to build for')
  parser.add_argument('--clang_dir', default='', help='Path to clang binaries')
  parser.add_argument('--disable_clang', action='store_true',
                      help='Use compilers other than clang for building')
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='Prints the output of the subprocesses')
  return parser.parse_args()


if __name__ == '__main__':
  sys.exit(main())
