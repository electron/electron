#!/usr/bin/env python

import argparse
import glob
import os
import shutil
import sys
import tarfile

from lib.config import PLATFORM, get_target_arch, s3_config
from lib.util import execute, safe_mkdir, scoped_cwd, s3put


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR    = os.path.join(SOURCE_ROOT, 'dist')
NODE_DIR    = os.path.join(SOURCE_ROOT, 'vendor', 'node')
OUT_DIR     = os.path.join(SOURCE_ROOT, 'out', 'R')

HEADERS_SUFFIX = [
  '.h',
  '.gypi',
]
HEADERS_DIRS = [
  'src',
  'deps/http_parser',
  'deps/zlib',
  'deps/uv',
  'deps/npm',
  'deps/mdb_v8',
]
HEADERS_FILES = [
  'common.gypi',
  'config.gypi',
]


def main():
  safe_mkdir(DIST_DIR)

  args = parse_args()
  node_headers_dir = os.path.join(DIST_DIR, 'node-{0}'.format(args.version))
  iojs_headers_dir = os.path.join(DIST_DIR, 'iojs-{0}'.format(args.version))
  iojs2_headers_dir = os.path.join(DIST_DIR,
                                   'iojs-{0}-headers'.format(args.version))

  copy_headers(node_headers_dir)
  create_header_tarball(node_headers_dir)
  copy_headers(iojs_headers_dir)
  create_header_tarball(iojs_headers_dir)
  copy_headers(iojs2_headers_dir)
  create_header_tarball(iojs2_headers_dir)

  # Upload node's headers to S3.
  bucket, access_key, secret_key = s3_config()
  upload_node(bucket, access_key, secret_key, args.version)


def parse_args():
  parser = argparse.ArgumentParser(description='upload sumsha file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      required=True)
  return parser.parse_args()


def copy_headers(dist_headers_dir):
  safe_mkdir(dist_headers_dir)

  # Copy standard node headers from node. repository.
  for include_path in HEADERS_DIRS:
    abs_path = os.path.join(NODE_DIR, include_path)
    for dirpath, _, filenames in os.walk(abs_path):
      for filename in filenames:
        extension = os.path.splitext(filename)[1]
        if extension not in HEADERS_SUFFIX:
          continue
        copy_source_file(os.path.join(dirpath, filename), NODE_DIR,
                         dist_headers_dir)
  for other_file in HEADERS_FILES:
    copy_source_file(os.path.join(NODE_DIR, other_file), NODE_DIR,
                     dist_headers_dir)

  # Copy V8 headers from chromium's repository.
  src = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor', 'download',
                    'libchromiumcontent', 'src')
  for dirpath, _, filenames in os.walk(os.path.join(src, 'v8')):
    for filename in filenames:
      extension = os.path.splitext(filename)[1]
      if extension not in HEADERS_SUFFIX:
        continue
      copy_source_file(os.path.join(dirpath, filename), src,
                       os.path.join(dist_headers_dir, 'deps'))


def create_header_tarball(dist_headers_dir):
  target = dist_headers_dir + '.tar.gz'
  with scoped_cwd(DIST_DIR):
    tarball = tarfile.open(name=target, mode='w:gz')
    tarball.add(os.path.relpath(dist_headers_dir))
    tarball.close()


def copy_source_file(source, start, destination):
  relative = os.path.relpath(source, start=start)
  final_destination = os.path.join(destination, relative)
  safe_mkdir(os.path.dirname(final_destination))
  shutil.copy2(source, final_destination)


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
