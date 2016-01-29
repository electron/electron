#!/usr/bin/env python

import contextlib
import os
import subprocess
import sys


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  natives = os.path.abspath(sys.argv[1])
  js_source_files = sys.argv[2:]

  call_js2c(natives, js_source_files)


def call_js2c(natives, js_source_files):
  js2c = os.path.join(SOURCE_ROOT, 'vendor', 'node', 'tools', 'js2c.py')
  src_dir = os.path.dirname(js_source_files[0])
  with scoped_cwd(src_dir):
    subprocess.check_call(
        [sys.executable, js2c, natives] +
        [os.path.basename(source) for source in js_source_files])


@contextlib.contextmanager
def scoped_cwd(path):
  cwd = os.getcwd()
  os.chdir(path)
  try:
    yield
  finally:
    os.chdir(cwd)


if __name__ == '__main__':
  sys.exit(main())
