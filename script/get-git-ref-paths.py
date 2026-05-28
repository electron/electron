#!/usr/bin/env python3

import os
import subprocess
import sys

# Resolve the on-disk locations of HEAD and packed-refs for this checkout so
# that BUILD.gn can register them as build graph inputs. In a plain clone both
# live under electron/.git/, but in a `git worktree` checkout .git is a file
# pointing at <common>/.git/worktrees/<name>; HEAD lives there while
# packed-refs is shared in the common dir. GN's read_file() does not follow
# that indirection, so we ask git to do it for us.

ELECTRON_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))


def rev_parse(flag):
  out = subprocess.check_output(['git', 'rev-parse', flag],
                                cwd=ELECTRON_DIR,
                                stderr=subprocess.PIPE,
                                universal_newlines=True).strip()
  return out if os.path.isabs(out) else os.path.join(ELECTRON_DIR, out)


try:
  git_dir = rev_parse('--git-dir')
  common_dir = rev_parse('--git-common-dir')
except (subprocess.CalledProcessError, OSError) as e:
  sys.stderr.write(f'get-git-ref-paths.py: not a git checkout '
                   f'(set override_electron_version): {e}\n')
  sys.exit(1)

for p in (os.path.join(common_dir, 'packed-refs'),
          os.path.join(git_dir, 'HEAD')):
  print(os.path.normpath(p).replace('\\', '/'))
