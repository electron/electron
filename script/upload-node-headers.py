#!/usr/bin/env python

import argparse
import glob
import os
import shutil
import sys

from lib.config import PLATFORM, get_target_arch, s3_config
from lib.util import safe_mkdir, scoped_cwd, s3put


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR    = os.path.join(SOURCE_ROOT, 'dist')
OUT_DIR     = os.path.join(SOURCE_ROOT, 'out', 'R')

def main():
  args = parse_args()

  # Upload node's headers to S3.
  bucket, access_key, secret_key = s3_config()
  upload_node(bucket, access_key, secret_key, args.version)


def parse_args():
  parser = argparse.ArgumentParser(description='upload sumsha file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      required=True)
  return parser.parse_args()


def upload_node(bucket, access_key, secret_key, version):
  with scoped_cwd(DIST_DIR):
    s3put(bucket, access_key, secret_key, DIST_DIR,
          'atom-shell/dist/{0}'.format(version), glob.glob('node-*.tar.gz'))
    s3put(bucket, access_key, secret_key, DIST_DIR,
          'atom-shell/dist/{0}'.format(version), glob.glob('iojs-*.tar.gz'))

  if PLATFORM == 'win32':
    if get_target_arch() == 'ia32':
      node_lib = os.path.join(DIST_DIR, 'node.lib')
      iojs_lib = os.path.join(DIST_DIR, 'win-x86', 'iojs.lib')
    else:
      node_lib = os.path.join(DIST_DIR, 'x64', 'node.lib')
      iojs_lib = os.path.join(DIST_DIR, 'win-x64', 'iojs.lib')
    safe_mkdir(os.path.dirname(node_lib))
    safe_mkdir(os.path.dirname(iojs_lib))

    # Copy atom.lib to node.lib and iojs.lib.
    atom_lib = os.path.join(OUT_DIR, 'node.dll.lib')
    shutil.copy2(atom_lib, node_lib)
    shutil.copy2(atom_lib, iojs_lib)

    # Upload the node.lib.
    s3put(bucket, access_key, secret_key, DIST_DIR,
          'atom-shell/dist/{0}'.format(version), [node_lib])

    # Upload the iojs.lib.
    s3put(bucket, access_key, secret_key, DIST_DIR,
          'atom-shell/dist/{0}'.format(version), [iojs_lib])


if __name__ == '__main__':
  sys.exit(main())
