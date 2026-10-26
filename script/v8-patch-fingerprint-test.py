#!/usr/bin/env python3

"""Focused tests for V8 patch fingerprints; requires Python and Git only."""

import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from contextlib import redirect_stdout
from unittest import mock

SPEC = importlib.util.spec_from_file_location(
    'v8_patch_fingerprint', Path(__file__).with_name('v8-patch-fingerprint.py'))
fingerprint = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fingerprint)


def run_main(*args):
  output = io.StringIO()
  with mock.patch('sys.argv', ['v8-patch-fingerprint.py', *args]), redirect_stdout(output):
    fingerprint.main()
  return output.getvalue().strip()


class RepositoryTests(unittest.TestCase):
  def setUp(self):
    # Cleanup must run after the test, not after setUp.
    temporary = tempfile.TemporaryDirectory(  # pylint: disable=consider-using-with
        prefix='electron-v8-fingerprint-')
    self.addCleanup(temporary.cleanup)
    self.repo = Path(temporary.name).resolve()
    self.git('-c', 'init.defaultRefFormat=files',
             'init', '--quiet', '--initial-branch=main')
    self.git('config', 'user.name', 'Fingerprint test')
    self.git('config', 'user.email', 'fingerprint@example.invalid')
    self.git('config', 'commit.gpgsign', 'false')
    self.git('config', 'core.autocrlf', 'false')
    self.git('config', 'core.filemode', 'true')
    (self.repo / 'first.txt').write_text('before\n', encoding='utf-8')
    (self.repo / 'second.txt').write_text('before\n', encoding='utf-8')
    self.commit('base')
    self.base = self.git('rev-parse', 'HEAD')

  def git(self, *args):
    return subprocess.check_output(
        ['git', '-C', str(self.repo), *args], stderr=subprocess.PIPE
    ).decode().strip()

  def commit(self, message):
    self.git('add', '--all')
    self.git('commit', '--quiet', '--message', message)

  def identity(self):
    return fingerprint.repository_fingerprint(self.repo, self.base)

  def test_equivalent_commit_histories(self):
    original = self.identity()
    for name in ('first.txt', 'second.txt'):
      (self.repo / name).write_text('after\n', encoding='utf-8')
    self.commit('combined patch')
    expected = self.identity()
    self.assertNotEqual(original, expected)
    self.assertRegex(expected, r'^-electron\.[0-9a-f]{16}$')
    for index, order in enumerate((('first.txt', 'second.txt'),
                                   ('second.txt', 'first.txt'))):
      self.git('checkout', '--quiet', '-b', f'split-{index}', self.base)
      for name in order:
        (self.repo / name).write_text('after\n', encoding='utf-8')
        self.commit(f'change {name}')
      self.assertEqual(expected, self.identity())
    self.git('commit', '--amend', '--quiet', '--message', 're-exported description')
    self.assertEqual(expected, self.identity())
    (self.repo / 'first.txt').write_text('uncommitted\n', encoding='utf-8')
    self.assertEqual(expected, self.identity())

  @unittest.skipIf(os.name == 'nt', 'Requires POSIX filenames and executable modes')
  def test_raw_diff_paths_modes_and_objects(self):
    unusual = 'space tab\tnewline\n.txt'
    (self.repo / unusual).write_text('added\n', encoding='utf-8')
    (self.repo / 'first.txt').write_text('changed\n', encoding='utf-8')
    (self.repo / 'first.txt').chmod(0o755)
    (self.repo / 'second.txt').unlink()
    self.commit('content, mode, addition and deletion')
    changes = {name: (before, after) for name, before, after in
               fingerprint.get_tree_diff(self.repo, self.base, 'HEAD')}
    self.assertEqual(set(changes), {'first.txt', 'second.txt', unusual})
    self.assertEqual(changes['first.txt'], (
        ('100644', self.git('rev-parse', f'{self.base}:first.txt')),
        ('100755', self.git('rev-parse', 'HEAD:first.txt'))))
    self.assertEqual(changes['second.txt'], (
        ('100644', self.git('rev-parse', f'{self.base}:second.txt')),
        ('000000', '0' * 40)))
    self.assertEqual(changes[unusual], (
        ('000000', '0' * 40),
        ('100644', self.git('rev-parse', f'HEAD:{unusual}'))))

  def test_gn_inputs_track_head_and_exported_series(self):
    patches = self.repo / 'exported-patches'
    patches.mkdir()
    manifest = patches / '.patches'
    patch = patches / 'example.patch'
    manifest.write_text('example.patch\n', encoding='utf-8')
    patch.write_text('exported patch\n', encoding='utf-8')
    with mock.patch.object(fingerprint, 'V8_ROOT', self.repo), \
         mock.patch.object(fingerprint, 'PATCH_ROOT', patches), \
         mock.patch.object(fingerprint, 'pinned_revision', return_value=self.base):
      dependencies = {Path(name) for name in json.loads(run_main('--inputs'))}
      self.assertEqual(dependencies, {
          self.repo / '.git' / 'HEAD', self.repo.parent / 'DEPS', manifest, patch})
      self.git('checkout', '--quiet', '--detach')
      self.assertEqual(dependencies, {Path(name) for name in json.loads(run_main('--inputs'))})
      additional = patches / 'additional.patch'
      additional.write_text('additional exported patch\n', encoding='utf-8')
      manifest.write_text('example.patch\nadditional.patch\n', encoding='utf-8')
      self.assertIn(str(additional), json.loads(run_main('--inputs')))

  def test_non_ancestor_has_actionable_error(self):
    self.git('checkout', '--quiet', '--orphan', 'unrelated')
    self.commit('unrelated root')
    with self.assertRaisesRegex(ValueError, 're-sync.*v8_revision') as context:
      self.identity()
    self.assertIn(self.base, str(context.exception))
    self.assertIn(str(self.repo), str(context.exception))
    self.assertIsInstance(context.exception.__cause__, subprocess.CalledProcessError)

class MetadataTests(unittest.TestCase):
  def setUp(self):
    # Cleanup must run after the test, not after setUp.
    temporary = tempfile.TemporaryDirectory(  # pylint: disable=consider-using-with
        prefix='electron-v8-metadata-')
    self.addCleanup(temporary.cleanup)
    self.root = Path(temporary.name)
    self.metadata_path = self.root / 'metadata.json'
    self.patch = self.root / 'example.patch'
    self.patch.write_bytes(b'patch contents\n')
    patcher = mock.patch.object(fingerprint, 'CACHE_METADATA', self.metadata_path)
    patcher.start()
    self.addCleanup(patcher.stop)
    self.base = 'a' * 40
    self.metadata = {
        'schema': fingerprint.SCHEMA,
        'base_revision': self.base,
        'patch_provenance': fingerprint.patch_provenance([self.patch]),
        'fingerprint': '-electron.0123456789abcdef',
    }

  def write_metadata(self, metadata):
    self.metadata_path.write_text(json.dumps(metadata), encoding='utf-8')

  def test_cached_metadata_validation(self):
    with self.assertRaisesRegex(ValueError, 'regenerate the source cache'):
      fingerprint.cached_fingerprint(self.base, [self.patch])

    self.write_metadata(self.metadata)
    self.assertEqual(fingerprint.cached_fingerprint(self.base, [self.patch]),
                     self.metadata['fingerprint'])

    cases = [
        ('schema', 999, 'unsupported.*schema'),
        ('base_revision', 'b' * 40, 'does not match Chromium DEPS'),
        ('patch_provenance', 'wrong', 'does not match the patch series'),
        ('fingerprint', '-electron.not-a-hash', 'invalid V8 patch fingerprint'),
    ]
    for field, value, message in cases:
      with self.subTest(field=field, value=value):
        self.write_metadata({**self.metadata, field: value})
        with self.assertRaisesRegex(ValueError, message):
          fingerprint.cached_fingerprint(self.base, [self.patch])


if __name__ == '__main__':
  unittest.main()
