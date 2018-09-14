"""Git helper functions.

Everything in here should be project agnostic, shouldn't rely on project's structure,
and make any assumptions about the passed arguments or calls outcomes.
"""

import os
import subprocess

from util import scoped_cwd


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


def apply(repo, patch_path, directory=None, index=False, reverse=False):
  args = ['git', 'apply',
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

  with scoped_cwd(repo):
    return_code = subprocess.call(args)
    applied_successfully = (return_code == 0)
    return applied_successfully


def get_patch(repo, commit_hash):
  args = ['git', 'diff-tree',
          '-p',
          commit_hash,
          '--'  # Explicitly tell Git that `commit_hash` is a revision, not a path.
          ]

  with scoped_cwd(repo):
    return subprocess.check_output(args)


def get_head_commit(repo):
  args = ['git', 'rev-parse', 'HEAD']

  with scoped_cwd(repo):
    return subprocess.check_output(args).strip()


def reset(repo):
  args = ['git', 'reset']

  with scoped_cwd(repo):
    subprocess.check_call(args)


def commit(repo, author, message):
  """ Commit whatever in the index is now."""

  # Let's setup committer info so git won't complain about it being missing.
  # TODO: Is there a better way to set committer's name and email?
  env = os.environ.copy()
  env['GIT_COMMITTER_NAME'] = 'Anonymous Committer'
  env['GIT_COMMITTER_EMAIL'] = 'anonymous@electronjs.org'

  args = ['git', 'commit',
          '--author', author,
          '--message', message
          ]

  with scoped_cwd(repo):
    return_code = subprocess.call(args, env=env)
    committed_successfully = (return_code == 0)
    return committed_successfully
