#!/usr/bin/env python

import argparse
import os
import subprocess
import sys

from lib.config import MIPS64EL_GCC, get_target_arch, build_env, \
                       enable_verbose_mode, is_verbose_mode
from lib.util import electron_gyp, import_vs_env


CONFIGURATIONS = ['Release', 'Debug']
SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')
LIBCC_SOURCE_ROOT = os.path.join(SOURCE_ROOT, 'vendor', 'libchromiumcontent')
LIBCC_DIST_MAIN = os.path.join(LIBCC_SOURCE_ROOT, 'dist', 'main')
GCLIENT_DONE = os.path.join(SOURCE_ROOT, '.gclient_done')


def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()
  if args.verbose:
    enable_verbose_mode()

  # Update the VS build env.
  import_vs_env(get_target_arch())

  # decide which ninja executable to use
  ninja_path = args.ninja_path
  if not ninja_path:
    ninja_path = os.path.join('vendor', 'depot_tools', 'ninja')
    if sys.platform == 'win32':
      ninja_path += '.exe'

  # decide how to invoke ninja
  ninja = [ninja_path]
  if is_verbose_mode():
    ninja.append('-v')

  if args.libcc:
    if ('D' not in args.configuration
        or not os.path.exists(GCLIENT_DONE)
        or not os.path.exists(os.path.join(LIBCC_DIST_MAIN, 'build.ninja'))):
      sys.stderr.write('--libcc should only be used when '
                       'libchromiumcontent was built with bootstrap.py -d '
                       '--debug_libchromiumcontent' + os.linesep)
      sys.exit(1)
    script = os.path.join(LIBCC_SOURCE_ROOT, 'script', 'build')
    subprocess.check_call([sys.executable, script, '-D', '-t',
                           get_target_arch()])
    subprocess.check_call(ninja + ['-C', LIBCC_DIST_MAIN])

  env = build_env()
  for config in args.configuration:
    build_path = os.path.join('out', config[0])
    build_args = ['-C', build_path, args.target]
    if args.compdb:
      build_args += ['-t', 'compdb', 'cxx', 'cc']
      compdb = open(r'compile_commands.json','w')
      ret = subprocess.call(ninja + build_args, env=env, stdout=compdb)
      compdb.close()
    else:
      ret = subprocess.call(ninja + build_args, env=env)
    if ret != 0:
      sys.exit(ret)


def parse_args():
  parser = argparse.ArgumentParser(description='Build project')
  parser.add_argument('-c', '--configuration',
                      help='Build with Release or Debug configuration',
                      nargs='+',
                      default=CONFIGURATIONS,
                      required=False)
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      default=False,
                      help='Verbose output')
  parser.add_argument('-t', '--target',
                      help='Build specified target',
                      default=electron_gyp()['project_name%'],
                      required=False)
  parser.add_argument('--libcc',
                      help=(
                        'Build libchromiumcontent first. Should be used only '
                        'when libchromiumcontent as built with boostrap.py '
                        '-d --debug_libchromiumcontent.'
                      ),
                      action='store_true', default=False)
  parser.add_argument('--ninja-path',
                      help='Path of ninja command to use.',
                      required=False)
  parser.add_argument('--compdb',
                      help=(
                        'Generate JSON compilation database. This will not '
                        'trigger actual build. '
                      ),
                      action='store_true', default=False)
  return parser.parse_args()


if __name__ == '__main__':
  sys.exit(main())
