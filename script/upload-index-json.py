#!/usr/bin/env python

import os
import sys

from lib.config import PLATFORM, s3_config
from lib.util import electron_gyp, execute, s3put, scoped_cwd


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']


def main():
  # Upload the index.json.
  with scoped_cwd(SOURCE_ROOT):
    if len(sys.argv) == 2 and sys.argv[1] == '-R':
      config = 'R'
    else:
      config = 'D'
    out_dir = os.path.join(SOURCE_ROOT, 'out', config)
    if sys.platform == 'darwin':
      electron = os.path.join(out_dir, '{0}.app'.format(PRODUCT_NAME),
                                'Contents', 'MacOS', PRODUCT_NAME)
    elif sys.platform == 'win32':
      electron = os.path.join(out_dir, '{0}.exe'.format(PROJECT_NAME))
    else:
      electron = os.path.join(out_dir, PROJECT_NAME)
    index_json = os.path.relpath(os.path.join(out_dir, 'index.json'))
    execute([electron,
             os.path.join('tools', 'dump-version-info.js'),
             index_json])

    bucket, access_key, secret_key = s3_config()
    s3put(bucket, access_key, secret_key, out_dir, 'atom-shell/dist',
          [index_json])


if __name__ == '__main__':
  sys.exit(main())
