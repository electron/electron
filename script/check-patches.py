#!/usr/bin/env python

import os
import argparse
import json
import sys

from lib import git
from lib.util import index_of_first
from lib.patches import (
  patch_filenames_for_dir,
  guess_base_commit,
  format_patch,
  split_patches,
  get_file_name,
)

def print_in_a_frame(message, padding = 2):
  size = len(message)
  padding_str = ' ' * padding * 2

  print('#' * (size + 2 + padding * 2 * 2))
  for _ in range(padding):
    print('#{}{}{}#'.format(padding_str, ' ' * size, padding_str))
  print('#{}{}{}#'.format(padding_str, message, padding_str))
  for _ in range(padding):
    print('#{}{}{}#'.format(padding_str, ' ' * size, padding_str))
  print('#' * (size + 2 + padding * 2 * 2))

def strip_atat_lines(line):
  """Strip @@ ... @@ lines to make them easier to compare"""
  if line.startswith('@@'):
    return line.split('@@', 2)[1].strip()
  return line

def filter_patch_line(l):
  """Filter out lines from the patches that contain the hashes. These can not be
  reliably used to compare two patches (the same changes might cause different
  hashes if the original patch is older and did not apply 100% cleanly)"""
  return not l.startswith('index ') and not len(l) == 0

def map_diffs(patch_lines):
  """Turns the lines of the patchfiles into a map, where the keys are the files
  that are changed by the patch (the entire diff --... line) and the associated
  values are the actual changes"""
  diffs = {}

  key = patch_lines[0].strip()
  diffs[key] = []
  for line in patch_lines[1:]:
    if line.startswith('diff -'):
      key = line.strip()
      diffs[key] = []
    else:
      diffs[key].append(line)

  return diffs

def get_patch_diffs(patch):
  """Filter out the beginning of the file, until the first diff -- line, do some
  additional filtering and then turn the file into a map of diffs"""
  start = index_of_first(patch, lambda l: l.startswith('diff -'))
  meaningful_patch = patch[start:]
  patch_lines = filter(filter_patch_line, meaningful_patch)
  patch_lines = map(strip_atat_lines, patch_lines)

  return map_diffs(patch_lines)

def get_patches_from_git(root, dirs):
  """Get the patches that are applied currently to the various repos. Very
  similar to how git-export-patches works, but does some processing with
  get_patch_diffs() to make things easier to compare"""
  patch_map = {}

  for _, repo in dirs.iteritems():
    repo_dir = os.path.normpath(os.path.join(root, repo))

    patch_range = guess_base_commit(repo_dir)[0]
    patch_data = format_patch(repo_dir, patch_range)
    patches = split_patches(patch_data)

    for patch in patches:
      patch_name = os.path.join(repo, get_file_name(patch))
      patch_map[patch_name] = get_patch_diffs(patch)

  return patch_map

def compare_diffs(a, b):
  """Compares two patches, checking the diffs (changes of one file in a patch).
  This expects the patches to be in the diff map form mentioned above"""
  if len(a) != len(b):
    return False

  try:
    for diff in a:
      if len(a[diff]) != len(b[diff]):
        return False

      for i in range(0, len(a[diff])):
        if a[diff][i].strip() != b[diff][i].strip():
          return False
  except:
    return False

  return True

def check_patches(root, dirs, inputs):
  """Iterates over the patch files it gets as input, and checks if they are
  equivalent to the ones already applied"""
  patches_from_git = get_patches_from_git(root, dirs)

  for patch in inputs:
    patch_relative_path = os.path.relpath(patch, root)
    patch_dir = os.path.dirname(patch_relative_path).replace('\\', '/')

    patch_name = os.path.join(dirs[patch_dir], os.path.basename(patch))

    with open(patch) as f:
      patch_from_file = get_patch_diffs(f.readlines())
      if not compare_diffs(patch_from_file, patches_from_git[patch_name]):
        return False

  return True


def parse_args():
  parser = argparse.ArgumentParser(description='Apply Electron patches')
  parser.add_argument('-c', '--config',
                      type=argparse.FileType('r'),
                      help='patches\' config(s) in the JSON format',
                      required=True)
  return parser.parse_args()

def main():
  args = parse_args()

  root_dir = os.path.normpath(os.path.join(os.getcwd(), "../.."))
  patchfiles = []
  for root, _, files in os.walk(os.path.join(os.getcwd(), "patches/common")):
    for filename in files:
      if filename.endswith('.patches'):
        with open(os.path.join(root, filename)) as f:
          for line in f.readlines():
            patchfiles.append(os.path.join(root, line.strip()))

  if not check_patches(root_dir, json.load(args.config), patchfiles):
    print_in_a_frame('Patches have changed, run `gclient sync` again.')
    sys.exit(1)

if __name__ == '__main__':
  main()
