#!/usr/bin/env python

import argparse
import atexit
import os
import shutil
import sys
import tarfile
import time

from subprocess import Popen, PIPE
from lib.util import execute_stdout

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR    = os.path.join(SOURCE_ROOT, 'dist')


def main():
  args = parse_args()
  header_dir = os.path.join(DIST_DIR, args.version)

  # Generate Headers
  script_path = os.path.join(SOURCE_ROOT, 'script', 'create-node-headers.py')
  execute_stdout([sys.executable, script_path, '--version', args.version,
                  '--directory', header_dir])

  # Launch server
  script_path = os.path.join(SOURCE_ROOT, 'node_modules', 'serve', 'bin', 'serve.js')
  server = Popen(['node', script_path, '--port=4321'], stdout=PIPE, cwd=DIST_DIR)
  def cleanup():
    server.kill()
  atexit.register(cleanup)

  time.sleep(1)

  # Generate Checksums
  script_path = os.path.join(SOURCE_ROOT, 'script', 'upload-node-checksums.py')
  execute_stdout([sys.executable, script_path, '--version', args.version,
                  '--dist-url', 'http://localhost:4321',
                  '--target-dir', header_dir])

  print("Point your npm config at 'http://localhost:4321'")
  server.wait()

def parse_args():
  parser = argparse.ArgumentParser(description='create node header tarballs')
  parser.add_argument('-v', '--version', help='Specify the version',
                      required=True)
  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
