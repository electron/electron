#!/usr/bin/env python

import errno
import os
import platform
import sys


BASE_URL = os.getenv('LIBCHROMIUMCONTENT_MIRROR') or \
    'https://s3.amazonaws.com/github-janky-artifacts/libchromiumcontent'
LIBCHROMIUMCONTENT_COMMIT = '31ff4384348b139d9e138d4df41b85a11d1a4dbb'

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


def s3_config():
  # TODO Remove ATOM_SHELL_* fallback values
  config = (os.environ.get('ELECTRON_S3_BUCKET', os.environ.get('ATOM_SHELL_S3_BUCKET', '')),
            os.environ.get('ELECTRON_S3_ACCESS_KEY',  os.environ.get('ATOM_SHELL_S3_ACCESS_KEY', '')),
            os.environ.get('ELECTRON_S3_SECRET_KEY', os.environ.get('ATOM_SHELL_S3_SECRET_KEY', '')))
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
