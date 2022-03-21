#!/usr/bin/env python
from __future__ import print_function
import argparse
import os
import sys

from lib.config import LINUX_BINARIES, PLATFORM
from lib.util import execute, get_out_dir, safe_mkdir

# It has to be done before stripping the binaries.
def copy_debug_from_binaries(directory, out_dir, target_cpu, compress):
  for binary in LINUX_BINARIES:
    binary_path = os.path.join(directory, binary)
    if os.path.isfile(binary_path):
      copy_debug_from_binary(binary_path, out_dir, target_cpu, compress)

def copy_debug_from_binary(binary_path, out_dir, target_cpu, compress):
  if PLATFORM == 'linux' and target_cpu in ('x86', 'arm', 'arm64'):
    # Skip because no objcopy binary on the given target.
    return    
  debug_name = get_debug_name(binary_path)
  cmd = ['objcopy', '--only-keep-debug']
  if compress:
    cmd.extend(['--compress-debug-sections'])
  cmd.extend([binary_path, os.path.join(out_dir, debug_name)])
  execute(cmd)

def get_debug_name(binary_path):
  return os.path.basename(binary_path) + '.debug'

def main():
  args = parse_args()
  safe_mkdir(args.out_dir)
  if args.file:
    copy_debug_from_binary(args.file, args.out_dir, args.target_cpu,
                           args.compress)
  else:
    copy_debug_from_binaries(args.directory, args.out_dir, args.target_cpu,
                             args.compress)

def parse_args():
  parser = argparse.ArgumentParser(description='Copy debug from binaries')
  parser.add_argument('-d', '--directory',
                      help='Path to the dir that contains files to copy',
                      default=get_out_dir(),
                      required=False)
  parser.add_argument('-f', '--file',
                      help='Path to a specific file to copy debug symbols',
                      required=False)
  parser.add_argument('-o', '--out-dir',
                      help='Path to the dir that will contain the debugs',
                      default=None,
                      required=True)
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      help='Prints the output of the subprocesses')
  parser.add_argument('--target-cpu',
                      default='',
                      required=False,
                      help='Target cpu of binaries to copy debug symbols')
  parser.add_argument('--compress',
                      action='store_true',
                      required=False,
                      help='Compress the debug symbols')

  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
