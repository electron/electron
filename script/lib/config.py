#!/usr/bin/env python3

import os
import sys

PLATFORM = {
  'cygwin': 'win32',
  'msys': 'win32',
  'darwin': 'darwin',
  'linux': 'linux',
  'linux2': 'linux',
  'win32': 'win32',
  'win': 'win32',
}[sys.platform]

verbose_mode = False


def get_platform_key():
  if 'MAS_BUILD' in os.environ:
    return 'mas'

  return PLATFORM


def get_target_arch():
  arch = os.environ.get('TARGET_ARCH')
  if arch is None:
    return 'x64'
  if arch == 'x86':
    return 'ia32'
  return arch

def set_verbose_mode(mode):
  print('Running in verbose mode')
  global verbose_mode
  verbose_mode = mode

def is_verbose_mode():
  return verbose_mode

def verbose_mode_print(output):
  if verbose_mode:
    print(output)

def get_zip_name(name, version, suffix=''):
  arch = get_target_arch()
  if arch == 'arm':
    arch += 'v7l'
  zip_name = f'{name}-{version}-{get_platform_key()}-{arch}'
  if suffix:
    zip_name += '-' + suffix
  return zip_name + '.zip'

def get_tar_name(name, version, suffix=''):
  arch = get_target_arch()
  if arch == 'arm':
    arch += 'v7l'
  zip_name = f'{name}-{version}-{get_platform_key()}-{arch}'
  if suffix:
    zip_name += '-' + suffix
  return zip_name + '.tar.xz'
