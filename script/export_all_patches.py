#!/usr/bin/env python

import argparse
import json
import os
import sys

from lib import git


def export_patches(dirs):
  for patch_dir, repo in dirs.iteritems():
    git.export_patches(repo=repo, out_dir=patch_dir)


def parse_args():
  parser = argparse.ArgumentParser(description='Export Electron patches')
  parser.add_argument('config', nargs='+',
                      type=argparse.FileType('r'),
                      help='patches\' config(s) in the JSON format')
  return parser.parse_args()


def main():
  configs = parse_args().config
  for config_json in configs:
    export_patches(json.load(config_json))


if __name__ == '__main__':
  main()
