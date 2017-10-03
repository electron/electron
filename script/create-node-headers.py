#!/usr/bin/env python

import argparse
import os
import shutil
import sys
import tarfile

from lib.util import safe_mkdir, scoped_cwd


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
  args = parse_args()

  safe_mkdir(args.directory)

  node_headers_dir = os.path.join(args.directory,
                                  'node-{0}'.format(args.version))
  iojs_headers_dir = os.path.join(args.directory,
                                  'iojs-{0}'.format(args.version))
  iojs2_headers_dir = os.path.join(args.directory,
                                   'iojs-{0}-headers'.format(args.version))

  copy_headers(node_headers_dir)
  create_header_tarball(args.directory, node_headers_dir)

  copy_headers(iojs_headers_dir)
  create_header_tarball(args.directory, iojs_headers_dir)

  copy_headers(iojs2_headers_dir)
  create_header_tarball(args.directory, iojs2_headers_dir)


def parse_args():
  parser = argparse.ArgumentParser(description='create node header tarballs')
  parser.add_argument('-v', '--version', help='Specify the version',
                      required=True)
  parser.add_argument('-d', '--directory', help='Specify the output directory',
                      default=DIST_DIR,
                      required=False)
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
  src = os.path.join(SOURCE_ROOT, 'vendor', 'download', 'libchromiumcontent',
                     'src')
  for dirpath, _, filenames in os.walk(os.path.join(src, 'v8')):
    for filename in filenames:
      extension = os.path.splitext(filename)[1]
      if extension not in HEADERS_SUFFIX:
        continue
      copy_source_file(os.path.join(dirpath, filename), src,
                       os.path.join(dist_headers_dir, 'deps'))


def create_header_tarball(directory, dist_headers_dir):
  target = dist_headers_dir + '.tar.gz'
  with scoped_cwd(directory):
    tarball = tarfile.open(name=target, mode='w:gz')
    tarball.add(os.path.relpath(dist_headers_dir))
    tarball.close()


def copy_source_file(source, start, destination):
  relative = os.path.relpath(source, start=start)
  final_destination = os.path.join(destination, relative)
  safe_mkdir(os.path.dirname(final_destination))
  shutil.copy2(source, final_destination)


if __name__ == '__main__':
  sys.exit(main())
