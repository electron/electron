#!/usr/bin/env python

import os
import sys

from lib.config import TARGET_PLATFORM
from lib.util import execute, rm_rf


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')
CHROMIUM_DIR = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor',
                            'download', 'libchromiumcontent', 'static_library')


def main(destination):
  rm_rf(destination)
  (project_name, product_name) = get_names_from_gyp()

  if TARGET_PLATFORM in ['darwin', 'linux']:
    # Generate the dump_syms tool.
    build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
    execute([sys.executable, build, '-c', 'R', '-t', 'dump_syms'])

    generate_breakpad_symbols = os.path.join(SOURCE_ROOT, 'tools', 'posix',
                                             'generate_breakpad_symbols.py')
    if TARGET_PLATFORM == 'darwin':
      start = os.path.join(OUT_DIR, '{0}.app'.format(product_name), 'Contents',
                           'MacOS', product_name)
    else:
      start = os.path.join(OUT_DIR, project_name)
    args = [
      '--build-dir={0}'.format(OUT_DIR),
      '--binary={0}'.format(start),
      '--symbols-dir={0}'.format(destination),
      '--libchromiumcontent-dir={0}'.format(CHROMIUM_DIR),
      '--clear',
      '--jobs=16',
    ]
  else:
    generate_breakpad_symbols = os.path.join(SOURCE_ROOT, 'tools', 'win',
                                             'generate_breakpad_symbols.py')
    args = [
      '--symbols-dir={0}'.format(destination),
      '--jobs=16',
      OUT_DIR,
      CHROMIUM_DIR,
    ]

  execute([sys.executable, generate_breakpad_symbols] + args)


def get_names_from_gyp():
  gyp = os.path.join(SOURCE_ROOT, 'atom.gyp')
  with open(gyp) as f:
    o = eval(f.read());
    return (o['variables']['project_name%'], o['variables']['product_name%'])


if __name__ == '__main__':
  sys.exit(main(sys.argv[1]))
