#!/usr/bin/env python3
"""Turn Chromium's in-tree WPT expectations into a baseline file.

Runs on the files wpt.py `prepare --revision pinned` copies out of a Chromium
checkout, with vpython3 and Chromium's vpython.toml. It reuses blinkpy, the
code Chromium's own run_wpt_tests.py uses, instead of re-implementing the
formats:

  * `WPTManifest.ensure_manifest` builds external/wpt/MANIFEST.json from the
  checked-in base manifest.
  * `WebTestFinder.skip_tests` gives the tests Chromium does not run (Skip
  lines in TestExpectations and NeverFixTests, manual tests).
  * `blinkpy.wpt_tests.test_loader.TestLoader.load_metadata` translates
  TestExpectations lines and `*-expected.txt` testharness baselines into
  wptrunner metadata, the same in-memory tree wptrunner is given on
  Chromium's bots.

Output (see diff.py for the format): for every test Chromium runs, the
statuses Chromium expects for the test and for each subtest that is not a
plain pass.
"""

import argparse
import json
import optparse
import os
import sys
import warnings

TEST_TYPES = ('testharness', 'reftest', 'crashtest')


def main():
  parser = argparse.ArgumentParser(
      description=__doc__.split('\n', maxsplit=1)[0])
  parser.add_argument('--chromium-src', required=True)
  parser.add_argument('--out', required=True, help='baseline JSON to write')
  parser.add_argument(
      '--skip-out',
      required=True,
      help='file to write the tests Chromium skips to (one URL per line)')
  parser.add_argument(
      '--runner-args-out',
      required=True,
      help='JSON file to write the browser switches Chromium uses to')
  args = parser.parse_args()

  src = os.path.abspath(args.chromium_src)
  # Chromium's vendored wpt tools go first: typ (added to sys.path by
  # blinkpy) has a conflicting top-level `tools` package, and blinkpy skips
  # adding the vendored tools when any sys.path entry is named "wpt", which
  # this script's own directory is.
  sys.path[:0] = [
      os.path.join(src, 'third_party', 'wpt_tools', 'wpt'),
      os.path.join(src, 'third_party', 'blink', 'tools')
  ]
  # blinkpy has a few invalid escape sequences that newer Pythons warn about.
  warnings.simplefilter('ignore', SyntaxWarning)

  # blinkpy is only importable once sys.path points into the checkout.
  # pylint: disable=import-outside-toplevel
  from blinkpy.common import path_finder
  path_finder.bootstrap_wpt_imports()
  from blinkpy.common.host import Host
  from blinkpy.w3c.wpt_manifest import WPTManifest
  from blinkpy.web_tests.controllers.web_test_finder import WebTestFinder
  from blinkpy.web_tests.models.test_expectations import TestExpectations
  from blinkpy.wpt_tests.product import HeadlessShell
  from blinkpy.wpt_tests.test_loader import TestLoader, _test_basename
  from tools.manifest import manifest as wptmanifest

  # The option values run_wpt_tests.py ends up with for a release
  # headless_shell run on Linux with no test arguments.
  options = optparse.Values({
      'configuration': 'Release',
      'target': 'Release',
      'no_expectations': False,
      'enable_sanitizer': False,
      'wpt_only': True,
      'manifest_update': True,
      'skipped': 'default',
      'skip_timeouts': False,
      'skip_failing_tests': False,
      'additional_expectations': [],
      'additional_driver_flag': [],
    'wrapper': [],
  })
  port = Host().port_factory.get('linux', options)
  WPTManifest.ensure_manifest(port)
  expectations = TestExpectations(port)

  tests_root = os.path.join(port.web_tests_dir(), 'external', 'wpt')
  manifest = wptmanifest.load_and_update(tests_root,
                                         os.path.join(tests_root,
                                                      'MANIFEST.json'),
                                         '/',
                                         update=False)

  # TestLoader.__init__ starts loading tests for a wptrunner run; only its
  # expectation translation is wanted here, so skip the constructor.
  loader = object.__new__(TestLoader)
  loader._port = port
  loader._expectations = expectations
  loader._tests_to_skip = []
  loader.sanitizer_mode = False
  run_info = {'os': 'linux', 'product': 'headless_shell', 'debug': False}

  items = []
  for _test_type, path, tests in manifest.itertypes(*TEST_TYPES):
    items.extend((path, test) for test in tests)
  blink_names = {f"external/wpt{test.id}": test.id for _, test in items}
  skipped = WebTestFinder(port, options).skip_tests([], list(blink_names),
                                                    expectations)

  baseline = {}
  metadata_by_path = {}
  for path, test in items:
    if f"external/wpt{test.id}" in skipped:
      continue
    if path not in metadata_by_path:
      metadata_by_path[path] = loader.load_metadata(run_info, manifest,
                                                    tests_root, path)[1]
    entry = {}
    metadata = metadata_by_path[path]
    # The in-memory metadata is keyed by the last URL path segment.
    node = metadata.get_test(_test_basename(test.id)) if metadata else None
    if node is not None:
      entry['expected'] = statuses(node)
      if node.has_key('expect_any_subtests'):
        entry['any_subtests'] = True
      subtests = {name: statuses(sub) for name, sub in node.subtests.items()}
      if subtests:
        entry['subtests'] = subtests
    baseline[test.id] = entry

  with open(args.out, 'w', encoding='utf-8') as f:
    json.dump({'tests': baseline}, f)
  with open(args.skip_out, 'w', encoding='utf-8') as f:
    f.writelines(f"{blink_names[name]}\n" for name in sorted(skipped))
  # The switches Chromium starts headless_shell with for these tests. A few
  # are specific to headless_shell and do nothing in Electron. The crash
  # reporter ones point into a build directory that does not exist here.
  binary_args = (HeadlessShell(port, options).additional_binary_args() +
                 port.additional_driver_flags())
  with open(args.runner_args_out, 'w', encoding='utf-8') as f:
    json.dump([arg for arg in binary_args if 'crash' not in arg], f)
  with_expectations = sum(1 for entry in baseline.values() if entry)
  print(f'{len(baseline)} tests, {with_expectations} with non-pass '
        f'expectations, {len(skipped)} skipped by Chromium')


def statuses(node):
  try:
    value = node.get('expected')
  except KeyError:
    return ['PASS', 'OK']
  # Sorted, because the order blinkpy gives varies between Python versions.
  return [value] if isinstance(value, str) else sorted(value)


if __name__ == '__main__':
  main()
