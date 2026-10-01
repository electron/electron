#!/usr/bin/env python3

"""Identify net committed source changes on top of Chromium's pinned V8."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re

from lib import git

ELECTRON_ROOT = Path(__file__).resolve().parent.parent
V8_ROOT = ELECTRON_ROOT.parent / 'v8'
SCHEMA = 1


def pinned_revision(deps):
  match = re.search(r"""['"]v8_revision['"]:\s*['"]([0-9a-f]{40,64})['"]""",
                    deps.read_text(encoding='utf-8'))
  if not match:
    raise ValueError(f'Cannot find the pinned V8 revision in {deps}')
  return match.group(1)


def hash_changes(object_format, changes):
  digest = hashlib.sha256()
  fields = [f'electron-v8-patch-effect-v{SCHEMA}', object_format]
  for name, before, after in sorted(changes):
    fields.extend([name, *before, *after])
  for field in fields:
    encoded = os.fsencode(field)
    digest.update(len(encoded).to_bytes(8, 'big'))
    digest.update(encoded)
  return '-electron.' + digest.hexdigest()[:16]


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--inputs', action='store_true')
  args = parser.parse_args()
  if not (V8_ROOT / '.git').exists():
    parser.error('A synced V8 Git checkout is required to generate the embedder string')
  base = pinned_revision(V8_ROOT.parent / 'DEPS')
  if args.inputs:
    inputs = git.get_ref_inputs(V8_ROOT)
    inputs.update([V8_ROOT.parent / 'DEPS', Path(git.__file__),
                   Path(git.__file__).with_name('patches.py')])
    print(json.dumps(sorted(str(path).replace('\\', '/') for path in inputs)))
  else:
    git.check_ancestor(V8_ROOT, base)
    changes = git.get_tree_diff(V8_ROOT, base,
                                git.get_commit_for_ref(V8_ROOT, 'HEAD^{tree}'))
    print(hash_changes(git.get_object_format(V8_ROOT), changes))


if __name__ == '__main__':
  main()
