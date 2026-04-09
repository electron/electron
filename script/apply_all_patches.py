#!/usr/bin/env python3

import argparse
import concurrent.futures
import json
import os
import subprocess
import warnings

from lib import git
from lib.patches import patch_from_dir

ELECTRON_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
THREEWAY = "ELECTRON_USE_THREE_WAY_MERGE_FOR_PATCHES" in os.environ

def apply_patches(target):
  repo = target.get('repo')
  if not os.path.exists(repo):
    warnings.warn(f'repo not found: {repo}')
    return
  patch_dir = target.get('patch_dir')
  git.import_patches(
    committer_email="scripts@electron",
    committer_name="Electron Scripts",
    output_prefix=f'[{os.path.basename(patch_dir)}] ',
    patch_data=patch_from_dir(patch_dir),
    repo=repo,
    threeway=THREEWAY,
  )

def is_roller_branch():
  try:
    branch = subprocess.check_output(
      ['git', '-C', ELECTRON_DIR, 'rev-parse', '--abbrev-ref', 'HEAD'],
      stderr=subprocess.DEVNULL,
    ).decode('utf-8').strip()
    return branch.startswith('roller/')
  except subprocess.CalledProcessError:
    return False

def apply_config(config):
  # Targets are independent git repos, so apply in parallel. The work is
  # subprocess-bound (git am), so threads are sufficient. On roller/
  # branches, patch conflicts are expected and interleaved failure output
  # from multiple repos is hard to read, so force sequential there.
  if is_roller_branch():
    max_workers = 1
  else:
    max_workers = max(1, (os.cpu_count() or 4) - 2)
  with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as pool:
    futures = {pool.submit(apply_patches, t): t for t in config}
    failed = []
    for f in concurrent.futures.as_completed(futures):
      try:
        f.result()
      except Exception as e:  # pylint: disable=broad-except
        failed.append((futures[f].get('repo'), e))
    if failed:
      for repo, e in failed:
        print(f'ERROR applying patches to {repo}: {e}')
      raise failed[0][1]

def parse_args():
  parser = argparse.ArgumentParser(description='Apply Electron patches')
  parser.add_argument('config', nargs='+',
                      type=argparse.FileType('r'),
                      help='patches\' config(s) in the JSON format')
  return parser.parse_args()


def main():
  for config_json in parse_args().config:
    apply_config(json.load(config_json))


if __name__ == '__main__':
  main()
