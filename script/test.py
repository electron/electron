#!/usr/bin/env python

import argparse
import os
import shutil
import subprocess
import sys

from lib.config import PLATFORM, enable_verbose_mode, get_target_arch
from lib.util import electron_gyp, execute, get_electron_version, rm_rf, \
                     update_electron_modules


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']


def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()
  config = args.configuration

  if args.verbose:
    enable_verbose_mode()

  spec_modules = os.path.join(SOURCE_ROOT, 'spec', 'node_modules')
  if args.rebuild_native_modules or not os.path.isdir(spec_modules):
    rebuild_native_modules(spec_modules,
                           os.path.join(SOURCE_ROOT, 'out', config))

  if sys.platform == 'darwin':
    electron = os.path.join(SOURCE_ROOT, 'out', config,
                              '{0}.app'.format(PRODUCT_NAME), 'Contents',
                              'MacOS', PRODUCT_NAME)
    resources_path = os.path.join(SOURCE_ROOT, 'out', config,
                                   '{0}.app'.format(PRODUCT_NAME), 'Contents',
                                   'Resources')
  elif sys.platform == 'win32':
    electron = os.path.join(SOURCE_ROOT, 'out', config,
                              '{0}.exe'.format(PROJECT_NAME))
    resources_path = os.path.join(SOURCE_ROOT, 'out', config)
  else:
    electron = os.path.join(SOURCE_ROOT, 'out', config, PROJECT_NAME)
    resources_path = os.path.join(SOURCE_ROOT, 'out', config)

  returncode = 0
  try:
    if args.use_instrumented_asar:
      install_instrumented_asar_file(resources_path)
    subprocess.check_call([electron, 'spec'] + sys.argv[1:])
  except subprocess.CalledProcessError as e:
    returncode = e.returncode
  except KeyboardInterrupt:
    returncode = 0

  if args.use_instrumented_asar:
    restore_uninstrumented_asar_file(resources_path)

  if os.environ.has_key('OUTPUT_TO_FILE'):
    output_to_file = os.environ['OUTPUT_TO_FILE']
    with open(output_to_file, 'r') as f:
      print f.read()
    rm_rf(output_to_file)


  return returncode


def parse_args():
  parser = argparse.ArgumentParser(description='Run Electron tests')
  parser.add_argument('--use_instrumented_asar',
                      help='Run tests with coverage instructed asar file',
                      action='store_true',
                      required=False)
  parser.add_argument('--rebuild_native_modules',
                      help='Rebuild native modules used by specs',
                      action='store_true',
                      required=False)
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      help='Prints the output of the subprocesses')
  parser.add_argument('-c', '--configuration',
                      help='Build configuration to run tests against',
                      default='D',
                      required=False)
  return parser.parse_args()


def install_instrumented_asar_file(resources_path):
  asar_path = os.path.join(resources_path, '{0}.asar'.format(PROJECT_NAME))
  uninstrumented_path = os.path.join(resources_path,
                                      '{0}-original.asar'.format(PROJECT_NAME))
  instrumented_path = os.path.join(SOURCE_ROOT, 'out', 'coverage',
                                      '{0}.asar'.format(PROJECT_NAME))
  shutil.move(asar_path, uninstrumented_path)
  shutil.move(instrumented_path, asar_path)


def restore_uninstrumented_asar_file(resources_path):
  asar_path = os.path.join(resources_path, '{0}.asar'.format(PROJECT_NAME))
  uninstrumented_path = os.path.join(resources_path,
                                      '{0}-original.asar'.format(PROJECT_NAME))
  os.remove(asar_path)
  shutil.move(uninstrumented_path, asar_path)


def run_python_script(script, *args):
  script_path = os.path.join(SOURCE_ROOT, 'script', script)
  return execute([sys.executable, script_path] + list(args))


def rebuild_native_modules(modules_path, out_dir):
  version = get_electron_version()

  run_python_script('create-node-headers.py',
                    '--version', version,
                    '--directory', out_dir)

  if PLATFORM == 'win32':
    iojs_lib = os.path.join(out_dir, 'Release', 'iojs.lib')
    atom_lib = os.path.join(out_dir, 'node.dll.lib')
    shutil.copy2(atom_lib, iojs_lib)

  update_electron_modules(os.path.dirname(modules_path), get_target_arch(),
                          os.path.join(out_dir, 'node-{0}'.format(version)))


if __name__ == '__main__':
  sys.exit(main())
