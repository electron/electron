#!/usr/bin/env python

import argparse
import errno
import subprocess
import sys
import os

from lib.util import *


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
NODE_VERSION = 'v0.10.9'
NODE_DIST_URL = 'https://gh-contractor-zcbenz.s3.amazonaws.com/node/dist'
IS_POSIX = (sys.platform != 'win32') and (sys.platform != 'cygwin')


def main():
  os.chdir(SOURCE_ROOT)

  args = parse_args()
  if not node_needs_update(args.version):
    return 0

  url, filename = get_node_url(args.url, args.version)
  directory = tempdir(prefix='atom-shell-')
  path = os.path.join(directory, filename)
  download('Download node', url, path)

  if IS_POSIX:
    root_name = 'node-{0}-{1}-x86'.format(args.version, sys.platform)
    member = os.path.join(root_name, 'bin', 'node')
    extract_tarball(path, member, directory)
    node_path = os.path.join(directory, member)

  copy_node(node_path)


def parse_args():
  parser = argparse.ArgumentParser(description='Update node binary')
  parser.add_argument('--version',
                      help='Version of node',
                      default=NODE_VERSION,
                      required=False)
  parser.add_argument('--url',
                      help='URL to download node',
                      default=NODE_DIST_URL,
                      required=False)
  return parser.parse_args()


def node_needs_update(target_version):
  try:
    node = os.path.join('node', 'node')
    if not IS_POSIX:
      node += '.exe'
    version = subprocess.check_output([node, '--version'])
    return version.strip() != target_version
  except OSError as e:
    if e.errno != errno.ENOENT:
      raise
    return True


def get_node_url(base_url, target_version):
  if IS_POSIX:
    distname = 'node-{0}-{1}-x86.tar.gz'.format(target_version, sys.platform)
  else:
    distname = 'node-{0}.exe'.format(target_version)
  return '{0}/{1}'.format(base_url, distname), distname


def copy_node(node_path):
  safe_mkdir('node')
  node = os.path.join('node', 'node')
  safe_unlink(node)
  os.rename(node_path, node)


if __name__ == '__main__':
  sys.exit(main())
