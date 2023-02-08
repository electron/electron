#!/usr/bin/env python3

import argparse
import json
import os

from lib import git


def export_patches(dirs, dry_run):
  for patch_dir, repo in dirs.items():
    if os.path.exists(repo):
      git.export_patches(repo=repo, out_dir=patch_dir, dry_run=dry_run)


def parse_args():
  parser = argparse.ArgumentParser(description='Export Electron patches')
  parser.add_argument('config', nargs='+',
                      type=argparse.FileType('r'),
                      help='patches\' config(s) in the JSON format')
  parser.add_argument("-d", "--dry-run",
    help="Checks whether the exported patches need to be updated.",
    default=False, action='store_true')
  return parser.parse_args()


def main():
  configs = parse_args().config
  dry_run = parse_args().dry_run
  for config_json in configs:
    export_patches(json.load(config_json), dry_run)


if __name__ == '__main__':
  main()
