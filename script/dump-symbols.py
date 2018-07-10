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
      #macOS has an additional helper app; provide the path to that binary also
      main_app = os.path.join(OUT_DIR, '{0}.app'.format(product_name),
                            'Contents', 'MacOS', product_name)
      helper_name = product_name + " Helper"
      helper_app = os.path.join(OUT_DIR, '{0}.app'.format(helper_name),
                            'Contents', 'MacOS', product_name + " Helper")
      binaries = [main_app, helper_app]
    else:
      binaries = [os.path.join(OUT_DIR, project_name)]
    args = [
      '--build-dir={0}'.format(OUT_DIR),
      '--symbols-dir={0}'.format(destination),
      '--libchromiumcontent-dir={0}'.format(CHROMIUM_DIR),
      '--clear',
      '--jobs=16',
    ]
    for binary in binaries:
      args += '--binary={0}'.format(binary),

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
