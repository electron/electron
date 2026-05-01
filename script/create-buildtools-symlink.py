#!/usr/bin/env python3
"""Create a buildtools symlink in the gclient root directory.

This enables gclient_paths.py to locate buildtools without needing the
CHROMIUM_BUILDTOOLS_PATH environment variable.
"""

import os
import sys


def main():
  electron_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
  src_dir = os.path.dirname(electron_dir)
  gclient_root = os.path.dirname(src_dir)

  source = os.path.join(src_dir, 'buildtools')
  link_name = os.path.join(gclient_root, 'buildtools')

  if not os.path.isdir(source):
    print(f'buildtools not found at {source}', file=sys.stderr)
    return 1

  # Already a symlink - verify it points to the right place.
  if os.path.islink(link_name):
    if os.path.realpath(link_name) == os.path.realpath(source):
      return 0
    os.remove(link_name)
  elif os.path.exists(link_name):
    # A real file or directory we shouldn't clobber.
    print(f'{link_name} already exists and is not a symlink',
          file=sys.stderr)
    return 1

  rel = os.path.relpath(source, gclient_root)
  try:
    os.symlink(rel, link_name, target_is_directory=True)
  except OSError as e:
    print(f'Failed to create symlink {link_name} -> {rel}: {e}',
          file=sys.stderr)
    if sys.platform == 'win32':
      print('On Windows, creating symlinks requires Developer Mode '
            'or administrator privileges.', file=sys.stderr)
    return 1

  return 0


if __name__ == '__main__':
  sys.exit(main())
