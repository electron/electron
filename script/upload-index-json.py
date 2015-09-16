#!/usr/bin/env python

import os
import sys

from lib.config import PLATFORM, s3_config
from lib.util import atom_gyp, execute, s3put, scoped_cwd


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR     = os.path.join(SOURCE_ROOT, 'out', 'D')

PROJECT_NAME = atom_gyp()['project_name%']
PRODUCT_NAME = atom_gyp()['product_name%']


def main():
  # Upload the index.json.
  with scoped_cwd(SOURCE_ROOT):
    if sys.platform == 'darwin':
      atom_shell = os.path.join(OUT_DIR, '{0}.app'.format(PRODUCT_NAME),
                                'Contents', 'MacOS', PRODUCT_NAME)
    elif sys.platform == 'win32':
      atom_shell = os.path.join(OUT_DIR, '{0}.exe'.format(PROJECT_NAME))
    else:
      atom_shell = os.path.join(OUT_DIR, PROJECT_NAME)
    index_json = os.path.relpath(os.path.join(OUT_DIR, 'index.json'))
    execute([atom_shell,
             os.path.join('tools', 'dump-version-info.js'),
             index_json])

    bucket, access_key, secret_key = s3_config()
    s3put(bucket, access_key, secret_key, OUT_DIR, 'atom-shell/dist',
          [index_json])


if __name__ == '__main__':
  sys.exit(main())
