#!/usr/bin/env python

# importing all dependencies
import os
import shutil
import subprocess
import sys

from lib.util import electron_gyp, rm_rf

# declare and define theenvironment variables.
SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']

# main function
def main():
  os.chdir(SOURCE_ROOT)

  config = 'D'
  if len(sys.argv) == 2 and sys.argv[1] == '-R':
    config = 'R'

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

  use_instrumented_asar = '--use-instrumented-asar' in sys.argv
  returncode = 0
  try:
    if use_instrumented_asar:
      install_instrumented_asar_file(resources_path)
    subprocess.check_call([electron, 'spec'] + sys.argv[1:])
  except subprocess.CalledProcessError as e:
    returncode = e.returncode
  except KeyboardInterrupt:
    returncode = 0

  if use_instrumented_asar:
    restore_uninstrumented_asar_file(resources_path)

  if os.environ.has_key('OUTPUT_TO_FILE'):
    output_to_file = os.environ['OUTPUT_TO_FILE']
    with open(output_to_file, 'r') as f:
      print f.read()
    rm_rf(output_to_file)


  return returncode


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


if __name__ == '__main__':
  sys.exit(main())
