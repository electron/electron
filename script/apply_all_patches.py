#!/usr/bin/env python

import argparse
import json
import sys

from lib import git
from lib.patches import patch_from_dir


def apply_patches(dirs):
  for patch_dir, repo in dirs.iteritems():
    git.am(repo=repo, patch_data=patch_from_dir(patch_dir),
      committer_name="Electron Scripts", committer_email="scripts@electron")


def parse_args():
  parser = argparse.ArgumentParser(description='Apply Electron patches')
  parser.add_argument('config', nargs='+',
                      type=argparse.FileType('r'),
                      help='patches\' config(s) in the JSON format')
  return parser.parse_args()


def main():
  configs = parse_args().config
  for config_json in configs:
    apply_patches(json.load(config_json))


if __name__ == '__main__':
  main()
