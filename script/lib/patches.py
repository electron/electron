#!/usr/bin/env python

import codecs
import os


def read_patch(patch_dir, patch_filename):
  """Read a patch from |patch_dir/filename| and amend the commit message with
  metadata about the patch file it came from."""
  ret = []
  patch_path = os.path.join(patch_dir, patch_filename)
  with codecs.open(patch_path, encoding='utf-8') as f:
    for l in f.readlines():
      if l.startswith('diff -'):
        ret.append('Patch-Filename: {}\n'.format(patch_filename))
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
