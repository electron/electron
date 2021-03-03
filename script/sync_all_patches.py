#!/usr/bin/env python

import argparse
import json
import os
import sys

from lib import git


def sync_patches(dirs):
  threeway = os.environ.get("ELECTRON_USE_THREE_WAY_MERGE_FOR_PATCHES")
  for patch_dir, repo in dirs.iteritems():
    git.sync_patches(repo=repo, patch_dir=patch_dir,
      threeway=threeway is not None)


def parse_args():
  parser = argparse.ArgumentParser(description='Sync Electron patches')
  parser.add_argument('config', nargs='+',
                      type=argparse.FileType('r'),
                      help='patches\' config(s) in the JSON format')
  return parser.parse_args()


def main():
  configs = parse_args().config
  for config_json in configs:
    sync_patches(json.load(config_json))


if __name__ == '__main__':
  main()
