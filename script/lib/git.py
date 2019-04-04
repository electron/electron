#!/usr/bin/env python

"""Git helper functions.

Everything here should be project agnostic: it shouldn't rely on project's
structure, or make assumptions about the passed arguments or calls' outcomes.
"""

import os
import subprocess


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
  command = ['git'] + root_args + ['am'] + args
  proc = subprocess.Popen(
      command,
      stdin=subprocess.PIPE)
  proc.communicate(patch_data)
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


def get_patch(repo, commit_hash):
  args = ['git', '-C', repo, 'diff-tree',
          '-p',
          commit_hash,
          '--'  # Explicitly tell Git `commit_hash` is a revision, not a path.
          ]

  return subprocess.check_output(args)


def get_head_commit(repo):
  args = ['git', '-C', repo, 'rev-parse', 'HEAD']

  return subprocess.check_output(args).strip()


def reset(repo):
  args = ['git', '-C', repo, 'reset']

  subprocess.check_call(args)


def commit(repo, author, message):
  """ Commit whatever in the index is now."""

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
