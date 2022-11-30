#!/usr/bin/env python3

import codecs
import os


def read_patch(patch_dir, patch_filename):
  """Read a patch from |patch_dir/filename| and amend the commit message with
  metadata about the patch file it came from."""
  ret = []
  added_filename_line = False
  patch_path = os.path.join(patch_dir, patch_filename)
  with codecs.open(patch_path, encoding='utf-8') as f:
    for l in f.readlines():
      line_has_correct_start = l.startswith('diff -') or l.startswith('---')
      if not added_filename_line and line_has_correct_start:
        ret.append('Patch-Filename: {}\n'.format(patch_filename))
        added_filename_line = True
      ret.append(l)
  return ''.join(ret)


def patch_from_dir(patch_dir):
  """Read a directory of patches into a format suitable for passing to
  'git am'"""
  with open(os.path.join(patch_dir, ".patches")) as f:
    patch_list = [l.rstrip('\n') for l in f.readlines()]

  return ''.join([
    read_patch(patch_dir, patch_filename)
    for patch_filename in patch_list
  ])
