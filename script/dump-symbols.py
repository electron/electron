#!/usr/bin/env python

import os
import sys

from lib.config import PLATFORM
from lib.util import electron_gyp, execute, rm_rf


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')
CHROMIUM_DIR = os.path.join(SOURCE_ROOT, 'vendor', 'download',
                            'libchromiumcontent', 'static_library')


def main(destination):
  rm_rf(destination)
  (project_name, product_name) = get_names_from_gyp()

  if PLATFORM in ['darwin', 'linux']:
    generate_breakpad_symbols = os.path.join(SOURCE_ROOT, 'tools', 'posix',
                                             'generate_breakpad_symbols.py')
    if PLATFORM == 'darwin':
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
      os.path.relpath(OUT_DIR),
    ]

  execute([sys.executable, generate_breakpad_symbols] + args)


def get_names_from_gyp():
  variables = electron_gyp()
  return (variables['project_name%'], variables['product_name%'])


if __name__ == '__main__':
  sys.exit(main(sys.argv[1]))
