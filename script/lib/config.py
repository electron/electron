#!/usr/bin/env python

import errno
import os
import platform
import sys


BASE_URL = os.getenv('LIBCHROMIUMCONTENT_MIRROR') or \
    'https://s3.amazonaws.com/brave-laptop-binaries/libchromiumcontent'
LIBCHROMIUMCONTENT_COMMIT = 'de868695083d30cdfe5adb07793a8d7ccb57f8d8'

PLATFORM = {
  'cygwin': 'win32',
  'darwin': 'darwin',
  'linux2': 'linux',
  'win32': 'win32',
}[sys.platform]

verbose_mode = False


def get_platform_key():
  if os.environ.has_key('MAS_BUILD'):
    return 'mas'
  else:
    return PLATFORM


def get_target_arch():
  try:
    target_arch_path = os.path.join(__file__, '..', '..', '..', 'vendor',
                                    'brightray', 'vendor', 'download',
                                    'libchromiumcontent', '.target_arch')
    with open(os.path.normpath(target_arch_path)) as f:
      return f.read().strip()
  except IOError as e:
    if e.errno != errno.ENOENT:
      raise

  return 'x64'


def get_chromedriver_version():
  return 'v2.21'

def get_env_var(name):
  value = os.environ.get('ELECTRON_' + name, '')
  if not value:
    # TODO Remove ATOM_SHELL_* fallback values
    value = os.environ.get('ATOM_SHELL_' + name, '')
    if value:
      print 'Warning: Use $ELECTRON_' + name + ' instead of $ATOM_SHELL_' + name
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
  print 'Running in verbose mode'
  global verbose_mode
  verbose_mode = True


def is_verbose_mode():
  return verbose_mode
