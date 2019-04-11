#!/usr/bin/env python

import os
import re
import subprocess


def read_patch(patch_dir, patch_filename):
  """Read a patch from |patch_dir/filename| and amend the commit message with
  metadata about the patch file it came from."""
  ret = []
  with open(os.path.join(patch_dir, patch_filename)) as f:
    for l in f.readlines():
      if l.startswith('diff -'):
        ret.append('Patch-Filename: {}\n'.format(patch_filename))
      ret.append(l)
  return ''.join(ret)


def patch_filenames_for_dir(patch_dir):
  """Read a directory of patches and return the list of patch filenames"""
  with open(os.path.join(patch_dir, ".patches")) as f:
    return [l.rstrip('\n') for l in f.readlines()]

def patch_from_dir(patch_dir):
  """Read a directory of patches into a format suitable for passing to
  'git am'"""
  patch_list = patch_filenames_for_dir(patch_dir)

  return ''.join([
    read_patch(patch_dir, patch_filename)
    for patch_filename in patch_list
  ])


def guess_base_commit(repo):
  """Guess which commit the patches might be based on"""
  args = [
    'git',
    '-C',
    repo,
    'reflog',
    'show',
    '--grep-reflog',
    'checkout:',
    '-1',
    '--no-abbrev',
  ]
  output = subprocess.check_output(args)
  m = re.search('HEAD@\{([0-9]+)\}', output)
  return [output[0:40], m.group(1)]


def format_patch(repo, since):
  args = [
    'git',
    '-C',
    repo,
    '-c',
    'core.attributesfile=' + os.path.join(os.path.dirname( \
                          os.path.realpath(__file__)), '.electron.attributes'),
    # Ensure it is not possible to match anything
    # Disabled for now as we have consistent chunk headers
    # '-c',
    # 'diff.electron.xfuncname=$^',
    'format-patch',
    '--keep-subject',
    '--no-stat',
    '--stdout',

    # Per RFC 3676 the signature is separated from the body by a line with
    # '-- ' on it. If the signature option is omitted the signature defaults
    # to the Git version number.
    '--no-signature',

    # The name of the parent commit object isn't useful information in this
    # context, so zero it out to avoid needless patch-file churn.
    '--zero-commit',

    # Some versions of git print out different numbers of characters in the
    # 'index' line of patches, so pass --full-index to get consistent
    # behaviour.
    '--full-index',
    since
  ]
  return subprocess.check_output(args)


def split_patches(patch_data):
  """Split a concatenated series of patches into N separate patches"""
  patches = []
  patch_start = re.compile('^From [0-9a-f]+ ')
  for line in patch_data.splitlines():
    if patch_start.match(line):
      patches.append([])
    patches[-1].append(line)
  return patches


def munge_subject_to_filename(subject):
  """Derive a suitable filename from a commit's subject"""
  if subject.endswith('.patch'):
    subject = subject[:-6]
  return re.sub(r'[^A-Za-z0-9-]+', '_', subject).strip('_').lower() + '.patch'


def get_file_name(patch):
  """Return the name of the file to which the patch should be written"""
  for line in patch:
    if line.startswith('Patch-Filename: '):
      return line[len('Patch-Filename: '):]
  # If no patch-filename header, munge the subject.
  for line in patch:
    if line.startswith('Subject: '):
      return munge_subject_to_filename(line[len('Subject: '):])


def remove_patch_filename(patch):
  """Strip out the Patch-Filename trailer from a patch's message body"""
  force_keep_next_line = False
  for i, l in enumerate(patch):
    is_patchfilename = l.startswith('Patch-Filename: ')
    next_is_patchfilename = i < len(patch) - 1 and \
                            patch[i+1].startswith('Patch-Filename: ')
    if not force_keep_next_line and \
       (is_patchfilename or (next_is_patchfilename and len(l.rstrip()) == 0)):
      pass # drop this line
    else:
      yield l
    force_keep_next_line = l.startswith('Subject: ')
