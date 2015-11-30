#!/usr/bin/env python

import os
import platform
import subprocess
import sys

from lib.config import get_target_arch, PLATFORM
from lib.util import get_host_arch


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)

  if PLATFORM != 'win32' and platform.architecture()[0] != '64bit':
    print 'Electron is required to be built on a 64bit machine'
    return 1

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
  env = os.environ.copy()
  if PLATFORM == 'linux' and target_arch != get_host_arch():
    env['GYP_CROSSCOMPILE'] = '1'
  elif PLATFORM == 'win32':
    env['GYP_MSVS_VERSION'] = '2013'
  python = sys.executable
  if sys.platform == 'cygwin':
    # Force using win32 python on cygwin.
    python = os.path.join('vendor', 'python_26', 'python.exe')
  gyp = os.path.join('vendor', 'brightray', 'vendor', 'gyp', 'gyp_main.py')
  gyp_pylib = os.path.join(os.path.dirname(gyp), 'pylib')
  # Avoid using the old gyp lib in system.
  env['PYTHONPATH'] = os.path.pathsep.join([gyp_pylib,
                                            env.get('PYTHONPATH', '')])
  # Whether to build for Mac App Store.
  if os.environ.has_key('MAS_BUILD'):
    mas_build = 1
  else:
    mas_build = 0
  defines = [
    '-Dlibchromiumcontent_component={0}'.format(component),
    '-Dtarget_arch={0}'.format(target_arch),
    '-Dhost_arch={0}'.format(get_host_arch()),
    '-Dlibrary=static_library',
    '-Dmas_build={0}'.format(mas_build),
  ]
  return subprocess.call([python, gyp, '-f', 'ninja', '--depth', '.',
                          'atom.gyp', '-Icommon.gypi'] + defines, env=env)


if __name__ == '__main__':
  sys.exit(main())
