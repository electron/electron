#!/usr/bin/env python3
from __future__ import print_function
import argparse
import os
import sys

from lib.config import LINUX_BINARIES, PLATFORM
from lib.util import execute, get_out_dir

def add_debug_link_into_binaries(directory, target_cpu, debug_dir):
  for binary in LINUX_BINARIES:
    binary_path = os.path.join(directory, binary)
    if os.path.isfile(binary_path):
      add_debug_link_into_binary(binary_path, target_cpu, debug_dir)

def add_debug_link_into_binary(binary_path, target_cpu, debug_dir):
  if PLATFORM == 'linux' and (target_cpu == 'x86' or target_cpu == 'arm' or
    target_cpu == 'arm64'):
    # Skip because no objcopy binary on the given target.
    return

  debug_name = get_debug_name(binary_path)
  # Make sure the path to the binary is not relative because of cwd param.
  real_binary_path = os.path.realpath(binary_path)
  cmd = ['objcopy', '--add-gnu-debuglink=' + debug_name, real_binary_path]
  execute(cmd, cwd=debug_dir)

def get_debug_name(binary_path):
  return os.path.basename(binary_path) + '.debug'

def main():
  args = parse_args()
  if args.file:
    add_debug_link_into_binary(args.file, args.target_cpu, args.debug_dir)
  else:
    add_debug_link_into_binaries(args.directory, args.target_cpu,
                                 args.debug_dir)

def parse_args():
  parser = argparse.ArgumentParser(description='Add debug link to binaries')
  parser.add_argument('-d', '--directory',
                      help='Path to the dir that contains files to add links',
                      default=get_out_dir(),
                      required=False)
  parser.add_argument('-f', '--file',
                      help='Path to a specific file to add debug link',
                      required=False)
  parser.add_argument('-s', '--debug-dir',
                      help='Path to the dir that contain the debugs',
                      default=None,
                      required=True)
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      help='Prints the output of the subprocesses')
  parser.add_argument('--target-cpu',
                      default='',
                      required=False,
                      help='Target cpu of binaries to add debug link')

  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
