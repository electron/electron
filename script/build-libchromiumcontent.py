#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import enable_verbose_mode, get_target_arch
from lib.util import execute_stdout
from bootstrap import get_libchromiumcontent_commit 


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
LIBCC_DIR = os.path.join(SOURCE_ROOT, 'vendor', 'libchromiumcontent')
GCLIENT_DONE_MARKER = os.path.join(SOURCE_ROOT, '.gclient_done')
LIBCC_COMMIT = get_libchromiumcontent_commit()


def update_gclient_done_marker():
  with open(GCLIENT_DONE_MARKER, 'wb') as f:
    f.write(LIBCC_COMMIT)


def libchromiumcontent_outdated():
  if not os.path.exists(GCLIENT_DONE_MARKER):
    return True
  with open(GCLIENT_DONE_MARKER, 'rb') as f:
    return f.read() != LIBCC_COMMIT


def main():
  os.chdir(LIBCC_DIR)

  args = parse_args()
  if args.verbose:
    enable_verbose_mode()

  # ./script/bootstrap
  # ./script/update -t x64
  # ./script/build --no_shared_library -t x64
  # ./script/create-dist -c static_library -t x64 --no_zip
  script_dir = os.path.join(LIBCC_DIR, 'script')
  bootstrap = os.path.join(script_dir, 'bootstrap')
  update = os.path.join(script_dir, 'update')
  build = os.path.join(script_dir, 'build')
  create_dist = os.path.join(script_dir, 'create-dist')
  if args.force_update or libchromiumcontent_outdated():
    execute_stdout([sys.executable, bootstrap])
    execute_stdout([sys.executable, update, '-t', args.target_arch])
    update_gclient_done_marker()
  if args.debug:
    execute_stdout([sys.executable, build, '-D', '-t', args.target_arch])
    execute_stdout([sys.executable, create_dist, '-c', 'shared_library',
                    '--no_zip', '--keep-debug-symbols',
                    '-t', args.target_arch])
  else:
    execute_stdout([sys.executable, build, '-R', '-t', args.target_arch])
    execute_stdout([sys.executable, create_dist, '-c', 'static_library',
                    '--no_zip', '-t', args.target_arch])


def parse_args():
  parser = argparse.ArgumentParser(description='Build libchromiumcontent')
  parser.add_argument('--target_arch',
                      help='Specify the arch to build for')
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='Prints the output of the subprocesses')
  parser.add_argument('-d', '--debug', action='store_true',
                      help='Build libchromiumcontent for debugging')
  parser.add_argument('--force-update', default=False, action='store_true',
                      help='Force gclient to update libchromiumcontent')
  return parser.parse_args()


if __name__ == '__main__':
  sys.exit(main())
