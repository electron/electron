#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import PLATFORM
from lib.gn import gn
from lib.util import execute, rm_rf

ELECTRON_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SOURCE_ROOT = os.path.abspath(os.path.dirname(ELECTRON_ROOT))
RELEASE_PATH = os.path.join('out', 'Release')

def main():
  args = parse_args()
  rm_rf(args.destination)
  source_root = os.path.abspath(args.source_root)
  build_path = os.path.join(source_root, args.build_dir)
  product_name = gn(build_path).args().get_string('electron_product_name')
  project_name = gn(build_path).args().get_string('electron_project_name')

  if PLATFORM in ['darwin', 'linux']:
    generate_breakpad_symbols = os.path.join(source_root, 'components', 'crash',
                                             'content', 'tools',
                                             'generate_breakpad_symbols.py')
    if PLATFORM == 'darwin':
      #macOS has an additional helper app; provide the path to that binary also
      main_app = os.path.join(build_path, '{0}.app'.format(product_name),
                            'Contents', 'MacOS', product_name)
      helper_name = product_name + " Helper"
      helper_app = os.path.join(build_path, '{0}.app'.format(helper_name),
                            'Contents', 'MacOS', product_name + " Helper")
      binaries = [main_app, helper_app]
    else:
      binaries = [os.path.join(build_path, project_name)]
    args = [
      '--build-dir={0}'.format(build_path),
      '--symbols-dir={0}'.format(args.destination),
      '--clear',
      '--jobs=16',
    ]
    for binary in binaries:
      args += '--binary={0}'.format(binary),

  else:
    generate_breakpad_symbols = os.path.join(ELECTRON_ROOT, 'tools', 'win',
                                             'generate_breakpad_symbols.py')
    args = [
      '--symbols-dir={0}'.format(args.destination),
      '--jobs=16',
      os.path.relpath(build_path),
    ]
  print [sys.executable, generate_breakpad_symbols] + args
  execute([sys.executable, generate_breakpad_symbols] + args)

def parse_args():
  parser = argparse.ArgumentParser(description='Create breakpad symbols')
  parser.add_argument('-b', '--build-dir',
                      help='Path to an Electron build folder. \
                          Relative to the --source-root.',
                      default=RELEASE_PATH,
                      required=False)
  parser.add_argument('-d', '--destination',
                      help='Path to save symbols to.',
                      default=None,
                      required=True)
  parser.add_argument('-s', '--source-root',
                      help='Path to the src folder.',
                      default=SOURCE_ROOT,
                      required=False)
  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
