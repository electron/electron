#!/usr/bin/env python3

from __future__ import print_function
import os
import sys

PLATFORM = {
  'cygwin': 'win32',
  'msys': 'win32',
  'darwin': 'darwin',
  'linux': 'linux',
  'linux2': 'linux',
  'win32': 'win32',
}[sys.platform]

LINUX_BINARIES = [
  'chrome-sandbox',
  'chrome_crashpad_handler',
  'electron',
  'libEGL.so',
  'libGLESv2.so',
  'libffmpeg.so',
  'libvk_swiftshader.so',
]

verbose_mode = False


def get_platform_key():
  if 'MAS_BUILD' in os.environ:
    return 'mas'

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
