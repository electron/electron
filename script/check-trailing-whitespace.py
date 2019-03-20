#!/usr/bin/env python

import os
import sys

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DOCS_DIR = os.path.join(SOURCE_ROOT, 'docs')

def main():
  os.chdir(SOURCE_ROOT)

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
    trailingWhiteSpaceFiles += hasTrailingWhiteSpace(path)

  print('Parsed through ' + str(len(filepaths)) +
        ' files within docs directory and its ' +
        str(totalDirs) + ' subdirectories.')
  print('Found ' + str(trailingWhiteSpaceFiles) +
        ' files with trailing whitespace.')
  return trailingWhiteSpaceFiles

def hasTrailingWhiteSpace(filepath):
  try:
    f = open(filepath, 'r')
    lines = f.read().splitlines()
  except KeyboardInterrupt:
    print('Keyboard interruption whle parsing. Please try again.')
  finally:
    f.close()

  for line in lines:
    if line != line.rstrip():
      print "Trailing whitespace in: " + filepath
      return True

  return False

if __name__ == '__main__':
  sys.exit(main())
