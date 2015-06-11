#!/usr/bin/env python

import os
import platform
import subprocess
import sys

from lib.config import get_target_arch


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  update_external_binaries()
  return update_gyp()


def update_external_binaries():
  uf = os.path.join('script', 'update-external-binaries.py')
  subprocess.check_call([sys.executable, uf])


def update_gyp():
  # Since gyp doesn't support specify link_settings for each configuration,
  # we are not able to link to different libraries in  "Debug" and "Release"
  # configurations.
  # In order to work around this, we decided to generate the configuration
  # for twice, one is to generate "Debug" config, the other one to generate
  # the "Release" config. And the settings are controlled by the variable
  # "libchromiumcontent_component" which is defined before running gyp.
  target_arch = get_target_arch()
  return (run_gyp(target_arch, 0) or run_gyp(target_arch, 1))


def run_gyp(target_arch, component):
  python = sys.executable
  if sys.platform == 'cygwin':
    # Force using win32 python on cygwin.
    python = os.path.join('vendor', 'python_26', 'python.exe')
  gyp = os.path.join('vendor', 'brightray', 'vendor', 'gyp', 'gyp_main.py')
  defines = [
    '-Dlibchromiumcontent_component={0}'.format(component),
    '-Dtarget_arch={0}'.format(target_arch),
    '-Dhost_arch={0}'.format(target_arch),
    '-Dlibrary=static_library',
  ]
  return subprocess.call([python, gyp, '-f', 'ninja', '--depth', '.',
                          'atom.gyp', '-Icommon.gypi'] + defines)


def get_host_arch():
  if platform.architecture()[0] == '32bit':
    return 'ia32'
  else:
    return 'x64'


if __name__ == '__main__':
  sys.exit(main())
