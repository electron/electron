#!/usr/bin/env python
import argparse
import os
import sys

from lib.util import execute, get_out_dir

LINUX_BINARIES_TO_STRIP = [
  'electron',
  'libffmpeg.so',
  'libGLESv2.so',
  'libEGL.so'
]

def strip_binaries(directory, target_cpu):
  for binary in LINUX_BINARIES_TO_STRIP:
    binary_path = os.path.join(directory, binary)
    if os.path.isfile(binary_path):
      strip_binary(binary_path, target_cpu)

def strip_binary(binary_path, target_cpu):
  if target_cpu == 'arm':
    strip = 'arm-linux-gnueabihf-strip'
  elif target_cpu == 'arm64':
    strip = 'aarch64-linux-gnu-strip'
  elif target_cpu == 'mips64el':
    strip = 'mips64el-redhat-linux-strip'
  else:
    strip = 'strip'
  execute([strip, binary_path])

def main():
  args = parse_args()
  print args
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
                      action='store_true',
                      help='Prints the output of the subprocesses')
  parser.add_argument('--target-cpu',
                      default='',
                      required=False,
                      help='Target cpu of binaries to strip')

  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
