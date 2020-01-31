#!/usr/bin/env python

from __future__ import print_function

import argparse
import os
import re
import shlex
import subprocess
import sys

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

def main():
  args = parse_args()

  if sys.platform == 'darwin':
    chromedriver_name = 'chromedriver'
    chromedriver_path = os.path.join(args.source_root, args.chromedriver_path, chromedriver_name)

  process = subprocess.Popen([chromedriver_path], stdout=subprocess.PIPE, universal_newlines=True)
  output = []

  for _ in range(0, 3):
    output.append(process.stdout.readline())

  process.kill()

  returncode = 0
  match = re.search("^Starting ChromeDriver [0-9]+.[0-9]+.[0-9]+.[0-9]+ .* on port [0-9]+$", output[0])

  if (match == None):
    returncode = 1

  print("returncode")

def parse_args():
  parser = argparse.ArgumentParser(description='Test ChromeDriver')
  parser.add_argument('--source-root',
                      default=SOURCE_ROOT,
                      required=False)
  parser.add_argument('--chromedriver-path',
                      default=None,
                      required=True)
  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())