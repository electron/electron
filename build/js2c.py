#!/usr/bin/env python3

import os
import subprocess
import sys

def main():
  js2c = sys.argv[1]
  natives = sys.argv[2]
  js_source_files = sys.argv[3:]

  subprocess.check_call(
    [js2c, natives] + js_source_files + ['--only-js'])

if __name__ == '__main__':
  sys.exit(main())