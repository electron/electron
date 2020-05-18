#!/usr/bin/env python

from __future__ import print_function
import errno
import os
import platform
import sys

# URL to the mips64el sysroot image.
MIPS64EL_SYSROOT_URL = 'https://github.com/electron' \
                     + '/debian-sysroot-image-creator/releases/download' \
                     + '/v0.5.0/debian_jessie_mips64-sysroot.tar.bz2'
# URL to the mips64el toolchain.
MIPS64EL_GCC = 'gcc-4.8.3-d197-n64-loongson'
MIPS64EL_GCC_URL = 'http://ftp.loongnix.org/toolchain/gcc/release/' \
                 + MIPS64EL_GCC + '.tar.gz'

BASE_URL = os.getenv('LIBCHROMIUMCONTENT_MIRROR') or \
    'https://s3.amazonaws.com/github-janky-artifacts/libchromiumcontent'

PLATFORM = {
  'cygwin': 'win32',
  'darwin': 'darwin',
  'linux': 'linux',
  'linux2': 'linux',
  'win32': 'win32',
}[sys.platform]

LINUX_BINARIES = [
  'electron',
  'chrome-sandbox',
  'libffmpeg.so',
  'libGLESv2.so',
  'libEGL.so',
  'swiftshader/libGLESv2.so',
  'swiftshader/libEGL.so',
  'libvk_swiftshader.so'
]

verbose_mode = False


def get_platform_key():
  if os.environ.has_key('MAS_BUILD'):
    return 'mas'
  else:
    return PLATFORM


def get_target_arch():
  arch = os.environ.get('TARGET_ARCH')
  if arch is None:
    return 'x64'
  return arch


def get_env_var(name):
  value = os.environ.get('ELECTRON_' + name, '')
  if not value:
    # TODO Remove ATOM_SHELL_* fallback values
    value = os.environ.get('ATOM_SHELL_' + name, '')
    if value:
      print('Warning: Use $ELECTRON_' + name +
            ' instead of $ATOM_SHELL_' + name)
  return value


def s3_config():
  config = (get_env_var('S3_BUCKET'),
            get_env_var('S3_ACCESS_KEY'),
            get_env_var('S3_SECRET_KEY'))
  message = ('Error: Please set the $ELECTRON_S3_BUCKET, '
             '$ELECTRON_S3_ACCESS_KEY, and '
             '$ELECTRON_S3_SECRET_KEY environment variables')
  assert all(len(c) for c in config), message
  return config


def enable_verbose_mode():
  print('Running in verbose mode')
  global verbose_mode
  verbose_mode = True


def is_verbose_mode():
  return verbose_mode


def get_zip_name(name, version, suffix=''):
  arch = get_target_arch()
  if arch == 'arm':
    arch += 'v7l'
  zip_name = '{0}-{1}-{2}-{3}'.format(name, version, get_platform_key(), arch)
  if suffix:
    zip_name += '-' + suffix
  return zip_name + '.zip'


def build_env():
  env = os.environ.copy()
  if get_target_arch() == "mips64el":
    SOURCE_ROOT = os.path.abspath(os.path.dirname(
                    os.path.dirname(os.path.dirname(__file__))))
    VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')
    gcc_dir = os.path.join(VENDOR_DIR, MIPS64EL_GCC)
    ldlib_dirs = [
      gcc_dir + '/usr/x86_64-unknown-linux-gnu/mips64el-redhat-linux/lib',
      gcc_dir + '/usr/lib64',
      gcc_dir + '/usr/mips64el-redhat-linux/lib64',
      gcc_dir + '/usr/mips64el-redhat-linux/sysroot/lib64',
      gcc_dir + '/usr/mips64el-redhat-linux/sysroot/usr/lib64',
    ]
    env['LD_LIBRARY_PATH'] = os.pathsep.join(ldlib_dirs)
    env['PATH'] = os.pathsep.join([gcc_dir + '/usr/bin', env['PATH']])
  return env
