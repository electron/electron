#!/usr/bin/env python

import argparse
import errno
import hashlib
import json
import os

from lib.config import PLATFORM, get_target_arch
from lib.util import add_exec_bit, download, extract_zip, rm_rf, \
                     safe_mkdir, tempdir

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

def parse_args():
  parser = argparse.ArgumentParser(
      description='Download binaries for Electron build')

  parser.add_argument('--base-url', required=False,
                      help="Base URL for all downloads")
  parser.add_argument('--force', action='store_true', default=False, required=False)

  return parser.parse_args()


def parse_config():
  config_path = os.path.join(SOURCE_ROOT, 'script', 'external-binaries.json')
  with open(config_path, 'r') as config_file:
    config = json.load(config_file)
    return config


def main():
  args = parse_args()
  config = parse_config()

  base_url = args.base_url if args.base_url is not None else config['baseUrl']
  version = config['version']
  output_dir = os.path.join(SOURCE_ROOT, 'external_binaries')
  version_file = os.path.join(output_dir, '.version')

  if (is_updated(version_file, version) and not args.force):
    return

  rm_rf(output_dir)
  safe_mkdir(output_dir)

  for binary in config['binaries']:
    if not binary_should_be_downloaded(binary):
      continue

    temp_path = download_binary(base_url, version, binary['url'], binary['sha'])

    # We assume that all binaries are in zip archives.
    extract_zip(temp_path, output_dir)

    # Hack alert. Set exec bit for sccache binaries.
    # https://bugs.python.org/issue15795
    if 'sccache' in binary['url']:
      add_exec_bit_to_sccache_binary(output_dir)

  with open(version_file, 'w') as f:
    f.write(version)


def is_updated(version_file, version):
  existing_version = ''
  try:
    with open(version_file, 'r') as f:
      existing_version = f.readline().strip()
  except IOError as e:
    if e.errno != errno.ENOENT:
      raise
  return existing_version == version


def binary_should_be_downloaded(binary):
  if 'platform' in binary and binary['platform'] != PLATFORM:
    return False

  if 'targetArch' in binary and binary['targetArch'] != get_target_arch():
    return False

  return True


def sha256(file_path):
  hash_256 = hashlib.sha256()
  with open(file_path, "rb") as f:
      for chunk in iter(lambda: f.read(4096), b""):
          hash_256.update(chunk)
  return hash_256.hexdigest()


def download_binary(base_url, version, binary_url, sha):
  full_url = '{0}/{1}/{2}'.format(base_url, version, binary_url)
  temp_path = download_to_temp_dir(full_url, filename=binary_url, sha=sha)
  return temp_path


def validate_sha(file_path, sha):
  downloaded_sha = sha256(file_path)
  if downloaded_sha != sha:
    raise Exception("SHA for external binary file {} does not match expected '{}' != '{}'".format(file_path, downloaded_sha, sha))


def download_to_temp_dir(url, filename, sha):
  download_dir = tempdir(prefix='electron-')
  file_path = os.path.join(download_dir, filename)
  download(text='Download ' + filename, url=url, path=file_path)
  validate_sha(file_path, sha)
  return file_path


def add_exec_bit_to_sccache_binary(binary_dir):
  binary_name = 'sccache'
  if PLATFORM == 'win32':
    binary_name += '.exe'

  binary_path = os.path.join(binary_dir, binary_name)
  add_exec_bit(binary_path)


if __name__ == '__main__':
  main()
