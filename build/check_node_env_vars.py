#!/usr/bin/env python3
# Copyright (c) 2026 Anthropic, PBC.
# Use of this source code is governed by the MIT license that can be
# found in the LICENSE file.
"""Fails the build when Node.js reads an environment variable that
shell/common/node_env_vars.json does not classify, so that a Node.js upgrade
cannot add one without a decision on how the nodeOptions fuse treats it.

Also checks the reverse (stale manifest entries) and that the kFuseGated and
kInherited lists in node_env_vars.cc match the manifest.
"""

import argparse
import glob
import json
import os
import re
import sys

ACTIONS = ('fuse-gated', 'inherited', 'handled', 'allowed')

JS_READ = re.compile(
    r"""\benv(?:\.|\[\s*['"])([A-Z][A-Z0-9_]+)""")
JS_DESTRUCTURE = re.compile(r'\{([^{}]*)\}\s*=\s*process\.env\b')
CC_READ = re.compile(
    r'\b(?:SafeGetenv|secure_getenv|uv_os_getenv|getenv|opt_getter|Get)'
    r'\(\s*"([A-Z][A-Z0-9_]+)"')
CC_LIST = re.compile(r'constexpr base::cstring_view (k\w+)\[\] = \{(.*?)\};',
                     re.S)
UPPER_IDENT = re.compile(r'\b[A-Z][A-Z0-9_]+\b')
PRINT_HELP = 'lib/internal/main/print_help.js'
PRINT_HELP_ENTRY = re.compile(r"\[\s*'([A-Z][A-Z0-9_]+)'\s*,\s*\{")


def read(path):
  with open(path, encoding='utf-8', errors='replace') as f:
    return f.read()


def scan(node_dir):
  found = {}

  def add(name, path, text, pos):
    if name not in found:
      line = text.count('\n', 0, pos) + 1
      found[name] = '%s:%d' % (os.path.relpath(path, node_dir), line)

  for path in sorted(glob.glob(os.path.join(node_dir, 'lib', '**', '*.js'),
                               recursive=True)):
    text = read(path)
    for m in JS_READ.finditer(text):
      add(m.group(1), path, text, m.start())
    for m in JS_DESTRUCTURE.finditer(text):
      for ident in UPPER_IDENT.findall(m.group(1)):
        add(ident, path, text, m.start())
    if os.path.relpath(path, node_dir).replace(os.sep, '/') == PRINT_HELP:
      for m in PRINT_HELP_ENTRY.finditer(text):
        add(m.group(1), path, text, m.start())
  for ext in ('cc', 'h'):
    for path in sorted(glob.glob(os.path.join(node_dir, 'src', '**',
                                              '*.' + ext), recursive=True)):
      text = read(path)
      for m in CC_READ.finditer(text):
        add(m.group(1), path, text, m.start())
  return found


def cc_lists(cc_path):
  text = read(cc_path)
  lists = {}
  for m in CC_LIST.finditer(text):
    lists[m.group(1)] = set(re.findall(r'"([A-Z0-9_]+)"', m.group(2)))
  return lists


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--node-dir', required=True)
  parser.add_argument('--manifest', required=True)
  parser.add_argument('--cc', required=True)
  parser.add_argument('--stamp')
  parser.add_argument('--print-found', action='store_true')
  args = parser.parse_args()

  found = scan(args.node_dir)
  if args.print_found:
    for name in sorted(found):
      print('%-36s %s' % (name, found[name]))
    return 0

  with open(args.manifest, encoding='utf-8') as f:
    manifest = json.load(f)
  manifest.pop('//', None)

  errors = []
  for name, entry in manifest.items():
    if entry.get('action') not in ACTIONS:
      errors.append('%s: "action" must be one of %s' %
                    (name, ', '.join(ACTIONS)))

  new = sorted(set(found) - set(manifest))
  if new:
    errors.append(
        'Node.js reads environment variables that %s does not list:\n%s\n'
        'Decide how the nodeOptions fuse should treat each one, add it to the '
        'manifest and, for "fuse-gated" or "inherited", to %s and '
        'docs/tutorial/fuses.md.' %
        (os.path.basename(args.manifest),
         '\n'.join('  %-34s %s' % (n, found[n]) for n in new),
         os.path.basename(args.cc)))

  stale = sorted(set(manifest) - set(found))
  if stale:
    errors.append('%s lists variables that Node.js no longer reads; remove '
                  'them:\n  %s' % (os.path.basename(args.manifest),
                                   '\n  '.join(stale)))

  lists = cc_lists(args.cc)
  for action, list_name in (('fuse-gated', 'kFuseGated'),
                            ('inherited', 'kInherited')):
    want = {n for n, e in manifest.items() if e.get('action') == action}
    have = lists.get(list_name, set())
    if want != have:
      diff = ['  +%s' % n for n in sorted(want - have)]
      diff += ['  -%s' % n for n in sorted(have - want)]
      errors.append('%s in %s does not match the "%s" entries in %s:\n%s' %
                    (list_name, os.path.basename(args.cc), action,
                     os.path.basename(args.manifest), '\n'.join(diff)))

  if errors:
    sys.stderr.write('\n\n'.join(errors) + '\n')
    return 1
  if args.stamp:
    with open(args.stamp, 'w', encoding='utf-8'):
      pass
  return 0


if __name__ == '__main__':
  sys.exit(main())
