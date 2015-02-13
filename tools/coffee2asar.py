#!/usr/bin/env python

import os
import shutil
import subprocess
import sys
import tempfile


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))


def main():
  archive = sys.argv[1]
  coffee_source_files = sys.argv[2:]

  output_dir = tempfile.mkdtemp()
  compile_coffee(coffee_source_files, output_dir)
  call_asar(archive, output_dir)
  shutil.rmtree(output_dir)


def compile_coffee(coffee_source_files, output_dir):
  for source_file in coffee_source_files:
    output_filename = os.path.splitext(source_file)[0] + '.js'
    output_path = os.path.join(output_dir, output_filename)
    call_compile_coffee(source_file, output_path)


def call_compile_coffee(source_file, output_filename):
  compile_coffee = os.path.join(SOURCE_ROOT, 'tools', 'compile-coffee.py')
  subprocess.check_call([sys.executable, compile_coffee, source_file,
                         output_filename])


def call_asar(archive, output_dir):
  js_dir = os.path.join(output_dir, 'atom')
  asar = os.path.join(SOURCE_ROOT, 'node_modules', 'asar', 'bin', 'asar')
  subprocess.check_call([find_node(), asar, 'pack', js_dir, archive])


def find_node():
  WINDOWS_NODE_PATHs = [
    'C:/Program Files (x86)/nodejs',
    'C:/Program Files/nodejs',
  ] + os.environ['PATH'].split(os.pathsep)

  if sys.platform in ['win32', 'cygwin']:
    for path in WINDOWS_NODE_PATHs:
      full_path = os.path.join(path, 'node.exe')
      if os.path.exists(full_path):
        return full_path
  return 'node'

if __name__ == '__main__':
  sys.exit(main())
