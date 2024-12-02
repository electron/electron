#!/usr/bin/env python3

import argparse
import os
import sys

from lib.config import set_verbose_mode, is_verbose_mode, verbose_mode_print
from lib.util import execute, get_linux_binaries, get_out_dir

def get_size(path):
  size = os.path.getsize(path)
  units = ["bytes", "KB", "MB", "GB"]
  for unit in units:
    if size < 1024:
      return f"{size:.2f} {unit}"
    size /= 1024
  raise ValueError("File size is too large to be processed")

def strip_binaries(directory, target_cpu):
  if not os.path.isdir(directory):
    verbose_mode_print('Directory ' + directory + ' does not exist.')
    return

  verbose_mode_print('Stripping binaries in ' + directory)
  for binary in get_linux_binaries():
    verbose_mode_print('\nStripping ' + binary)
    binary_path = os.path.join(directory, binary)
    if os.path.isfile(binary_path):
      strip_binary(binary_path, target_cpu)

def strip_binary(binary_path, target_cpu):
  if target_cpu == 'arm':
    strip = 'arm-linux-gnueabihf-strip'
  elif target_cpu == 'arm64':
    strip = 'aarch64-linux-gnu-strip'
  else:
    strip = 'strip'
  
  strip_args = [strip,
                '--discard-all',
                '--strip-debug',
                '--preserve-dates',
                binary_path]
  if (is_verbose_mode()):
    strip_args.insert(1, '--verbose')
  verbose_mode_print('Binary size before stripping: ' + 
                     str(get_size(binary_path)))
  execute(strip_args)
  verbose_mode_print('Binary size after stripping: ' +
                     str(get_size(binary_path)))

def main():
  args = parse_args()
  set_verbose_mode(args.verbose)
  if args.file:
    strip_binary(args.file, args.target_cpu)
  else:
    strip_binaries(args.directory, args.target_cpu)

def parse_args():
  parser = argparse.ArgumentParser(description='Strip linux binaries')
  parser.add_argument('-d', '--directory',
                      help='Path to the dir that contains files to strip.',
                      default=get_out_dir(),
                      required=False)
  parser.add_argument('-f', '--file',
                      help='Path to a specific file to strip.',
                      required=False)
  parser.add_argument('-v', '--verbose',
                      default=False,
                      action='store_true',
                      help='Prints the output of the subprocesses')
  parser.add_argument('--target-cpu',
                      default='',
                      required=False,
                      help='Target cpu of binaries to strip')

  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
