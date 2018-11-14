#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import PLATFORM, enable_verbose_mode, is_verbose_mode
from lib.util import get_electron_branding, execute, rm_rf, get_out_dir, \
                     SRC_DIR

ELECTRON_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SOURCE_ROOT = os.path.abspath(os.path.dirname(ELECTRON_ROOT))
RELEASE_PATH = get_out_dir()

def main():
  args = parse_args()
  if args.verbose:
    enable_verbose_mode()
  rm_rf(args.destination)
  source_root = os.path.abspath(args.source_root)
  build_path = os.path.join(source_root, args.build_dir)
  (project_name, product_name) = get_names_from_branding()

  if PLATFORM in ['darwin', 'linux']:

    if PLATFORM == 'darwin':
      #macOS has an additional helper app; provide the path to that binary also
      main_app = os.path.join(build_path, '{0}.app'.format(product_name),
                            'Contents', 'MacOS', product_name)
      helper_name = product_name + " Helper"
      helper_app = os.path.join(build_path, '{0}.app'.format(helper_name),
                            'Contents', 'MacOS', product_name + " Helper")
      binaries = [main_app, helper_app]
      for binary in binaries:
        generate_posix_symbols(binary, source_root, build_path,
                                        args.destination)
    else:
      binary = os.path.join(build_path, project_name)
      generate_posix_symbols(binary, source_root, build_path,
                                      args.destination)

  else:
    generate_breakpad_symbols = os.path.join(ELECTRON_ROOT, 'tools', 'win',
                                             'generate_breakpad_symbols.py')
    args = [
      '--symbols-dir={0}'.format(args.destination),
      '--jobs=16',
      os.path.relpath(build_path),
    ]
    if is_verbose_mode():
      args += ['-v']
    #Make sure msdia140.dll is in the path (needed for dump_syms.exe)
    env = os.environ.copy()
    msdia140_dll_path =  os.path.join(SRC_DIR, 'third_party', 'llvm-build',
                                      'Release+Asserts', 'bin')
    env['PATH'] = os.path.pathsep.join(
        [env.get('PATH', '')] + [msdia140_dll_path])
    execute([sys.executable, generate_breakpad_symbols] + args, env)

def get_names_from_branding():
  variables = get_electron_branding()
  return (variables['project_name'], variables['product_name'])

def generate_posix_symbols(binary, source_root, build_dir, destination):
  generate_breakpad_symbols = os.path.join(source_root, 'components', 'crash',
                                          'content', 'tools',
                                          'generate_breakpad_symbols.py')
  args = [
    '--build-dir={0}'.format(build_dir),
    '--symbols-dir={0}'.format(destination),
    '--jobs=16',
    '--binary={0}'.format(binary),
  ]
  if is_verbose_mode():
    args += ['--verbose']
  execute([sys.executable, generate_breakpad_symbols] + args)

def parse_args():
  parser = argparse.ArgumentParser(description='Create breakpad symbols')
  parser.add_argument('-b', '--build-dir',
                      help='Path to an Electron build folder.',
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
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      help='Prints the output of the subprocesses')
  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
