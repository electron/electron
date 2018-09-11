import os
import subprocess
import sys

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
EXTERNAL_BINARIES_DIR = os.path.join(SOURCE_ROOT, 'external_binaries')

SUPPORTED_PLATFORMS = {
  'cygwin': 'windows',
  'darwin': 'mac',
  'linux2': 'linux',
  'win32': 'windows',
}

def is_platform_supported(platform):
  return platform in SUPPORTED_PLATFORMS


def get_binary_path():
  platform = sys.platform
  if not is_platform_supported(platform):
    return None

  platform_dir = SUPPORTED_PLATFORMS[platform]

  path = os.path.join(EXTERNAL_BINARIES_DIR, 'sccache')
  if platform_dir == 'windows':
    path += '.exe'

  return path


def run(*args):
  binary_path = get_binary_path()
  if binary_path is None:
    raise Exception('No sccache binary found for the current platform.')

  call_args = [binary_path] + list(args)
  return subprocess.call(call_args)
