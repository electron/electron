#!/usr/bin/env python

import argparse
import glob
import os
import shutil
import sys
import tarfile

from lib.config import TARGET_PLATFORM
from lib.util import execute, safe_mkdir, scoped_cwd, s3_config, s3put


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR    = os.path.join(SOURCE_ROOT, 'dist')
NODE_DIR    = os.path.join(SOURCE_ROOT, 'vendor', 'node')
OUT_DIR     = os.path.join(SOURCE_ROOT, 'out', 'Release')

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
  dist_headers_dir = os.path.join(DIST_DIR, 'node-{0}'.format(args.version))

  copy_headers(dist_headers_dir)
  create_header_tarball(dist_headers_dir)

  # Upload node's headers to S3.
  bucket, access_key, secret_key = s3_config()
  upload_node(bucket, access_key, secret_key, args.version)

  # Upload the SHASUMS.txt.
  execute([sys.executable,
           os.path.join(SOURCE_ROOT, 'script', 'upload-checksums.py'),
           '-v', args.version])


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

  if TARGET_PLATFORM == 'win32':
    # Generate the node.lib.
    build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
    execute([sys.executable, build, '-c', 'Release', '-t', 'generate_node_lib'])

    # Upload the 32bit node.lib.
    node_lib = os.path.join(OUT_DIR, 'node.lib')
    s3put(bucket, access_key, secret_key, OUT_DIR,
          'atom-shell/dist/{0}'.format(version), [node_lib])

    # Upload the fake 64bit node.lib.
    touch_x64_node_lib()
    node_lib = os.path.join(OUT_DIR, 'x64', 'node.lib')
    s3put(bucket, access_key, secret_key, OUT_DIR,
          'atom-shell/dist/{0}'.format(version), [node_lib])

    # Upload the index.json
    with scoped_cwd(SOURCE_ROOT):
      atom_shell = os.path.join(OUT_DIR, 'atom.exe')
      index_json = os.path.relpath(os.path.join(OUT_DIR, 'index.json'))
      execute([atom_shell,
               os.path.join('script', 'dump-version-info.js'),
               index_json])
      s3put(bucket, access_key, secret_key, OUT_DIR, 'atom-shell/dist',
            [index_json])


def touch_x64_node_lib():
  x64_dir = os.path.join(OUT_DIR, 'x64')
  safe_mkdir(x64_dir)
  with open(os.path.join(x64_dir, 'node.lib'), 'w+') as node_lib:
    node_lib.write('Invalid library')


if __name__ == '__main__':
  sys.exit(main())
