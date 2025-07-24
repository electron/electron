#!/usr/bin/env python3

import argparse
import os
import subprocess
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

def has_debug_info(binary_path, target_cpu):
  """Check if the binary still contains .debug_info section using readelf

  Returns:
    True: Binary contains .debug_info section (not properly stripped)
    False: Binary does not contain .debug_info section (properly stripped)
    None: Could not verify (readelf failed or not found)
  """
  if target_cpu == 'arm':
    readelf = 'arm-linux-gnueabihf-readelf'
  elif target_cpu == 'arm64':
    readelf = 'aarch64-linux-gnu-readelf'
  else:
    readelf = 'readelf'

  try:
    result = subprocess.run([readelf, '-S', binary_path], 
                          capture_output=True, text=True, check=True)
    # Successfully ran readelf, check if .debug_info is present
    return '.debug_info' in result.stdout
  except subprocess.CalledProcessError as e:
    verbose_mode_print(f'Warning: readelf failed for {binary_path}: {e}')
    return None
  except FileNotFoundError:
    print(f'Could not verify stripping: {readelf} not found')
    sys.exit(0)

def verify_stripped(binary_path, target_cpu):
  """Verify that debug info has been successfully removed

  Returns:
    True: Binary is properly stripped (no debug info found)
    False: Binary is not properly stripped (debug info found)
    None: Could not verify stripping status
  """
  debug_info_result = has_debug_info(binary_path, target_cpu)

  if debug_info_result is True:
    print(f'ERROR: {binary_path} still contains debug info after stripping')
    return False
  if debug_info_result is False:
    verbose_mode_print(f'Verified: {binary_path} properly stripped')
    return True
  verbose_mode_print(f'Could not verify: {binary_path}')
  return None

def strip_binaries(directory, target_cpu):
  if not os.path.isdir(directory):
    verbose_mode_print('Directory ' + directory + ' does not exist.')
    return

  verbose_mode_print('Stripping binaries in ' + directory)
  failed_count = 0
  for binary in get_linux_binaries():
    binary_path = os.path.join(directory, binary)
    if os.path.isfile(binary_path):
      verbose_mode_print(f'Stripping {binary}')
      try:
        strip_binary(binary_path, target_cpu)
      except SystemExit:
        failed_count += 1
        continue

  if failed_count > 0:
    print(f'ERROR: {failed_count} binaries failed strip verification!')
    sys.exit(1)

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
  verbose_mode_print(f'Stripping {binary_path} (before: {get_size(binary_path)})')
  execute(strip_args)
  verbose_mode_print(f'Stripped {binary_path} (after: {get_size(binary_path)})')

  # Verify that debug info has been successfully removed
  verification_result = verify_stripped(binary_path, target_cpu)
  if verification_result is False:
    sys.exit(1)
  elif verification_result is None:
    verbose_mode_print(f'Warning: could not verify stripping for {binary_path}')

def verify_only_binaries(directory, target_cpu):
  """Only verify that binaries in directory are stripped, don't strip them"""
  if not os.path.isdir(directory):
    verbose_mode_print('Directory ' + directory + ' does not exist.')
    return

  verbose_mode_print('Verifying binaries in ' + directory)
  failed_count = 0
  unverified_count = 0
  for binary in get_linux_binaries():
    binary_path = os.path.join(directory, binary)
    if os.path.isfile(binary_path):
      verbose_mode_print(f'Verifying {binary}')
      verification_result = verify_stripped(binary_path, target_cpu)
      if verification_result is False:
        failed_count += 1
      elif verification_result is None:
        unverified_count += 1

  if failed_count > 0:
    print(f'ERROR: {failed_count} binaries not properly stripped')
    sys.exit(1)
  elif unverified_count > 0:
    print(f'Could not verify {unverified_count} binaries, but no failures detected')
  else:
    print('All binaries properly stripped')

def main():
  args = parse_args()
  set_verbose_mode(args.verbose)
  try:
    if args.verify_only:
      if args.file:
        # Verify single file
        verification_result = verify_stripped(args.file, args.target_cpu)
        if verification_result is False:
          sys.exit(1)
        elif verification_result is None:
          print(f'Could not verify stripping for {args.file}')
        else:
          print(f'{args.file} is properly stripped')
      else:
        # Verify all binaries in directory
        verify_only_binaries(args.directory, args.target_cpu)
    elif args.file:
      strip_binary(args.file, args.target_cpu)
    else:
      strip_binaries(args.directory, args.target_cpu)
  except SystemExit as e:
    # Re-raise the exit code
    sys.exit(e.code)

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
  parser.add_argument('--verify-only',
                      default=False,
                      action='store_true',
                      help='Only verify that binaries are stripped, do not strip them')

  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
