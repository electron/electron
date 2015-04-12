#!/usr/bin/env python

import os
import sys

from lib.config import PLATFORM, s3_config
from lib.util import execute, s3put, scoped_cwd


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR     = os.path.join(SOURCE_ROOT, 'out', 'R')


def main():
  # Upload the index.json.
  with scoped_cwd(SOURCE_ROOT):
    atom_shell = os.path.join(OUT_DIR, 'atom')
    if PLATFORM == 'win32':
      atom_shell += '.exe'
    index_json = os.path.relpath(os.path.join(OUT_DIR, 'index.json'))
    execute([atom_shell,
             os.path.join('tools', 'dump-version-info.js'),
             index_json])

    bucket, access_key, secret_key = s3_config()
    s3put(bucket, access_key, secret_key, OUT_DIR, 'atom-shell/dist',
          [index_json])


if __name__ == '__main__':
  sys.exit(main())
