#!/usr/bin/env python

"""Git helper functions.

Everything here should be project agnostic: it shouldn't rely on project's
structure, or make assumptions about the passed arguments or calls' outcomes.
"""

from __future__ import unicode_literals

import io
import os
import posixpath
import re
import subprocess
import sys


def is_repo_root(path):
  path_exists = os.path.exists(path)
  if not path_exists:
    return False

  git_folder_path = os.path.join(path, '.git')
  git_folder_exists = os.path.exists(git_folder_path)

  return git_folder_exists


def get_repo_root(path):
  """Finds a closest ancestor folder which is a repo root."""
  norm_path = os.path.normpath(path)
  norm_path_exists = os.path.exists(norm_path)
  if not norm_path_exists:
    return None

  if is_repo_root(norm_path):
    return norm_path

  parent_path = os.path.dirname(norm_path)

  # Check if we're in the root folder already.
  if parent_path == norm_path:
    return None

  return get_repo_root(parent_path)


def am(repo, patch_data, threeway=False, directory=None, exclude=None,
    committer_name=None, committer_email=None):
  args = []
  if threeway:
    args += ['--3way']
  if directory is not None:
    args += ['--directory', directory]
  if exclude is not None:
    for path_pattern in exclude:
      args += ['--exclude', path_pattern]

  root_args = ['-C', repo]
  if committer_name is not None:
    root_args += ['-c', 'user.name=' + committer_name]
  if committer_email is not None:
    root_args += ['-c', 'user.email=' + committer_email]
  root_args += ['-c', 'commit.gpgsign=false']
  command = ['git'] + root_args + ['am'] + args
  proc = subprocess.Popen(
      command,
      stdin=subprocess.PIPE)
  proc.communicate(patch_data.encode('utf-8'))
  if proc.returncode != 0:
    raise RuntimeError("Command {} returned {}".format(command,
      proc.returncode))


def apply_patch(repo, patch_path, directory=None, index=False, reverse=False):
  args = ['git', '-C', repo, 'apply',
          '--ignore-space-change',
          '--ignore-whitespace',
          '--whitespace', 'fix'
          ]
  if directory:
    args += ['--directory', directory]
  if index:
    args += ['--index']
  if reverse:
    args += ['--reverse']
  args += ['--', patch_path]

  return_code = subprocess.call(args)
  applied_successfully = (return_code == 0)
  return applied_successfully


def import_patches(repo, **kwargs):
  """same as am(), but we save the upstream HEAD so we can refer to it when we
  later export patches"""
  update_ref(
      repo=repo,
      ref='refs/patches/upstream-head',
      newvalue='HEAD'
  )
  am(repo=repo, **kwargs)


def get_patch(repo, commit_hash):
  args = ['git', '-C', repo, 'diff-tree',
          '-p',
          commit_hash,
          '--'  # Explicitly tell Git `commit_hash` is a revision, not a path.
          ]

  return subprocess.check_output(args).decode('utf-8')


def get_head_commit(repo):
  args = ['git', '-C', repo, 'rev-parse', 'HEAD']

  return subprocess.check_output(args).decode('utf-8').strip()


def update_ref(repo, ref, newvalue):
  args = ['git', '-C', repo, 'update-ref', ref, newvalue]

  return subprocess.check_call(args)


def reset(repo):
  args = ['git', '-C', repo, 'reset']

  subprocess.check_call(args)


def commit(repo, author, message):
  """Commit whatever in the index is now."""

  # Let's setup committer info so git won't complain about it being missing.
  # TODO: Is there a better way to set committer's name and email?
  env = os.environ.copy()
  env['GIT_COMMITTER_NAME'] = 'Anonymous Committer'
  env['GIT_COMMITTER_EMAIL'] = 'anonymous@electronjs.org'

  args = ['git', '-C', repo, 'commit',
          '--author', author,
          '--message', message
          ]

  return_code = subprocess.call(args, env=env)
  committed_successfully = (return_code == 0)
  return committed_successfully

def get_upstream_head(repo):
  args = [
    'git',
    '-C',
    repo,
    'rev-parse',
    '--verify',
    'refs/patches/upstream-head',
  ]
  return subprocess.check_output(args).decode('utf-8').strip()

def get_commit_count(repo, commit_range):
  args = [
    'git',
    '-C',
    repo,
    'rev-list',
    '--count',
    commit_range
  ]
  return int(subprocess.check_output(args).decode('utf-8').strip())

def guess_base_commit(repo):
  """Guess which commit the patches might be based on"""
  try:
    upstream_head = get_upstream_head(repo)
    num_commits = get_commit_count(repo, upstream_head + '..')
    return [upstream_head, num_commits]
  except subprocess.CalledProcessError:
    args = [
      'git',
      '-C',
      repo,
      'describe',
      '--tags',
    ]
    return subprocess.check_output(args).decode('utf-8').rsplit('-', 2)[0:2]


def format_patch(repo, since):
  args = [
    'git',
    '-C',
    repo,
    '-c',
    'core.attributesfile='
    + os.path.join(
        os.path.dirname(os.path.realpath(__file__)),
        'electron.gitattributes',
    ),
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
  return subprocess.check_output(args).decode('utf-8')


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
    next_is_patchfilename = i < len(patch) - 1 and patch[i + 1].startswith(
      'Patch-Filename: '
    )
    if not force_keep_next_line and (
      is_patchfilename or (next_is_patchfilename and len(l.rstrip()) == 0)
    ):
      pass  # drop this line
    else:
      yield l
    force_keep_next_line = l.startswith('Subject: ')


def export_patches(repo, out_dir, patch_range=None, dry_run=False):
  if patch_range is None:
    patch_range, num_patches = guess_base_commit(repo)
    sys.stderr.write(
        "Exporting {} patches in {} since {}\n".format(num_patches, repo, patch_range[0:7])
    )
  patch_data = format_patch(repo, patch_range)
  patches = split_patches(patch_data)

  try:
    os.mkdir(out_dir)
  except OSError:
    pass

  if dry_run:
    # If we're doing a dry run, iterate through each patch and see if the newly
    # exported patch differs from what exists. Report number of mismatched
    # patches and fail if there's more than one.
    patch_count = 0
    for patch in patches:
      filename = get_file_name(patch)
      filepath = posixpath.join(out_dir, filename)
      existing_patch = io.open(filepath, 'r', encoding='utf-8').read()
      formatted_patch = (
        '\n'.join(remove_patch_filename(patch)).rstrip('\n') + '\n'
      )
      if formatted_patch != existing_patch:
        patch_count += 1
    if patch_count > 0:
      sys.stderr.write(
        "Patches in {} not up to date: {} patches need update\n".format(
          out_dir, patch_count
        )
      )
      exit(1)
  else:
    # Remove old patches so that deleted commits are correctly reflected in the
    # patch files (as a removed file)
    for p in os.listdir(out_dir):
      if p.endswith('.patch'):
        os.remove(posixpath.join(out_dir, p))
    with io.open(
      posixpath.join(out_dir, '.patches'),
      'w',
      newline='\n',
      encoding='utf-8',
    ) as pl:
      for patch in patches:
        filename = get_file_name(patch)
        file_path = posixpath.join(out_dir, filename)
        formatted_patch = (
          '\n'.join(remove_patch_filename(patch)).rstrip('\n') + '\n'
        )
        with io.open(
          file_path, 'w', newline='\n', encoding='utf-8'
        ) as f:
          f.write(formatted_patch)
        pl.write(filename + '\n')

