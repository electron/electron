#!/usr/bin/env python

from __future__ import print_function
import argparse
import os
import sys

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DOCS_DIR = os.path.join(SOURCE_ROOT, 'docs')

def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()

  filepaths = []
  totalDirs = 0
  try:
    for root, dirs, files in os.walk(DOCS_DIR):
      totalDirs += len(dirs)
      for f in files:
        if f.endswith('.md'):
          filepaths.append(os.path.join(root, f))
  except KeyboardInterrupt:
    print('Keyboard interruption. Please try again.')
    return

  trailingWhiteSpaceFiles = 0
  for path in filepaths:
    trailingWhiteSpaceFiles += hasTrailingWhiteSpace(path, args.fix)

  print('Parsed through ' + str(len(filepaths)) +
        ' files within docs directory and its ' +
        str(totalDirs) + ' subdirectories.')
  print('Found ' + str(trailingWhiteSpaceFiles) +
        ' files with trailing whitespace.')
  return trailingWhiteSpaceFiles

def hasTrailingWhiteSpace(filepath, fix):
  try:
    f = open(filepath, 'r')
    lines = f.read().splitlines()
  except KeyboardInterrupt:
    print('Keyboard interruption whle parsing. Please try again.')
  finally:
    f.close()

  fixed_lines = []
  for num, line in enumerate(lines):
    fixed_lines.append(line.rstrip() + '\n')
    if not fix and line != line.rstrip():
      print("Trailing whitespace on line {} in file: {}".format(
          num + 1, filepath))
      return True
  if fix:
    with open(filepath, 'w') as f:
      print(fixed_lines)
      f.writelines(fixed_lines)

  return False

def parse_args():
  parser = argparse.ArgumentParser(
                      description='Check for trailing whitespace in md files')
  parser.add_argument('-f', '--fix',
                      help='Automatically fix trailing whitespace issues',
                      action='store_true')
  return parser.parse_known_args()[0]

if __name__ == '__main__':
  sys.exit(main())
