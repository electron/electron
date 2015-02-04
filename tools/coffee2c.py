#!/usr/bin/env python

import os
import subprocess
import sys


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))


def main():
  natives = sys.argv[1]
  coffee_source_files = sys.argv[2:]

  output_dir = os.path.dirname(natives)
  js_source_files = compile_coffee(coffee_source_files, output_dir)
  call_js2c(natives, js_source_files)


def compile_coffee(coffee_source_files, output_dir):
  js_source_files = []
  for source_file in coffee_source_files:
    output_filename = os.path.splitext(source_file)[0] + '.js'
    output_path = os.path.join(output_dir, output_filename)
    js_source_files.append(output_path)
    call_compile_coffee(source_file, output_path)
  return js_source_files


def call_compile_coffee(source_file, output_filename):
  compile_coffee = os.path.join(SOURCE_ROOT, 'tools', 'compile-coffee.py')
  subprocess.check_call([sys.executable, compile_coffee, source_file,
                         output_filename])


def call_js2c(natives, js_source_files):
  js2c = os.path.join(SOURCE_ROOT, 'vendor', 'node', 'tools', 'js2c.py')
  subprocess.check_call([sys.executable, js2c, natives] + js_source_files)


if __name__ == '__main__':
  sys.exit(main())
