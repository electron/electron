#!/usr/bin/env python3

"""Identify net committed source changes on top of Chromium's pinned V8."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ELECTRON_ROOT = Path(__file__).resolve().parent.parent
V8_ROOT = ELECTRON_ROOT.parent / 'v8'
PATCH_ROOT = ELECTRON_ROOT / 'patches' / 'v8'
CACHE_METADATA = V8_ROOT / '.electron-patch-fingerprint.json'
SCHEMA = 1


def run_git(repo, *args):
  return subprocess.check_output(
      ['git', '--no-optional-locks', '-C', str(repo), *args])


def get_tree_diff(repo, before, after):
  """Return changed paths and their before/after (mode, object ID) pairs."""
  records = run_git(repo, 'diff', '--raw', '-z', '--no-renames', '--abbrev=64',
                    before, after).split(b'\0')
  changes = []
  for index in range(0, len(records) - 1, 2):
    before_mode, after_mode, before_id, after_id, _status = (
        records[index].decode('ascii').split())
    changes.append((os.fsdecode(records[index + 1]),
                    (before_mode[1:], before_id),
                    (after_mode, after_id)))
  return changes


def get_ref_inputs(repo):
  """Return paths whose modification can change HEAD, including packed refs."""
  repo = Path(repo).resolve()
  inputs = set()
  git_paths = ['HEAD', 'packed-refs']
  symbolic = run_git(repo, 'rev-parse', '--symbolic-full-name', 'HEAD').decode().strip()
  if symbolic != 'HEAD':
    git_paths.append(symbolic)
  for name in git_paths:
    path = Path(run_git(repo, 'rev-parse', '--git-path', name).decode().strip())
    if not path.is_absolute():
      path = repo / path
    # A packed branch can acquire a loose ref without changing packed-refs.
    while not path.exists():
      path = path.parent
    inputs.add(path.resolve())
  return inputs


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


def patch_inputs():
  manifest = PATCH_ROOT / '.patches'
  return [manifest] + [
      PATCH_ROOT / name for name in manifest.read_text(encoding='utf-8').splitlines()
      if name]


def patch_provenance(inputs):
  digest = hashlib.sha256()
  for path in inputs:
    for field in (path.name.encode('utf-8'),
                  path.read_bytes().replace(b'\r\n', b'\n')):
      digest.update(len(field).to_bytes(8, 'big'))
      digest.update(field)
  return digest.hexdigest()


def repository_fingerprint(base):
  run_git(V8_ROOT, 'merge-base', '--is-ancestor', base, 'HEAD')
  head_tree = run_git(V8_ROOT, 'rev-parse', '--verify', 'HEAD^{tree}').decode().strip()
  changes = get_tree_diff(V8_ROOT, base, head_tree)
  object_format = run_git(V8_ROOT, 'rev-parse', '--show-object-format').decode().strip()
  return hash_changes(object_format, changes)


def cached_fingerprint(base, inputs):
  if not CACHE_METADATA.is_file():
    raise ValueError(
        'V8 Git metadata is unavailable and the source cache has no V8 patch '
        'fingerprint metadata; regenerate the source cache from a synced checkout')
  metadata = json.loads(CACHE_METADATA.read_text(encoding='utf-8'))
  if not isinstance(metadata, dict):
    raise ValueError('The source cache contains invalid V8 fingerprint metadata')
  if metadata.get('schema') != SCHEMA:
    raise ValueError('The source cache uses an unsupported V8 fingerprint schema')
  if metadata.get('base_revision') != base:
    raise ValueError('The source cache V8 fingerprint does not match Chromium DEPS')
  if metadata.get('patch_provenance') != patch_provenance(inputs):
    raise ValueError('The source cache V8 fingerprint does not match the patch series')
  fingerprint = metadata.get('fingerprint')
  if not isinstance(fingerprint, str) or not re.fullmatch(
      r'-electron\.[0-9a-f]{16}', fingerprint):
    raise ValueError('The source cache contains an invalid V8 patch fingerprint')
  return fingerprint


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  mode = parser.add_mutually_exclusive_group()
  mode.add_argument('--inputs', action='store_true')
  mode.add_argument('--write-ci-cache', action='store_true',
                    help='Record the Git derived identity before CI removes V8 Git metadata')
  args = parser.parse_args()
  base = pinned_revision(V8_ROOT.parent / 'DEPS')
  has_git = (V8_ROOT / '.git').exists()
  if args.write_ci_cache:
    if not has_git:
      parser.error('--write-ci-cache requires a synced V8 Git checkout')
    fingerprint = repository_fingerprint(base)
    metadata = {
        'schema': SCHEMA,
        'base_revision': base,
        'patch_provenance': patch_provenance(patch_inputs()),
        'fingerprint': fingerprint,
    }
    with CACHE_METADATA.open('w', encoding='utf-8', newline='\n') as output:
      json.dump(metadata, output, sort_keys=True)
      output.write('\n')
    print(fingerprint)
    return

  cache_inputs = [] if has_git else patch_inputs()
  fingerprint = None if has_git else cached_fingerprint(base, cache_inputs)
  if args.inputs:
    inputs = get_ref_inputs(V8_ROOT) if has_git else {
        CACHE_METADATA, *cache_inputs}
    inputs.add(V8_ROOT.parent / 'DEPS')
    print(json.dumps(sorted(str(path).replace('\\', '/') for path in inputs)))
  else:
    print(repository_fingerprint(base) if has_git else fingerprint)


if __name__ == '__main__':
  main()
