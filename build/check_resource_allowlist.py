#!/usr/bin/env python3
"""Verifies that every grit resource referenced by the electron binary is
present in at least one of the .pak files we ship.

The resource allowlist is the set of resource IDs referenced by the binary's
link inputs (see //electron:electron_resource_allowlist). Any allowlisted ID
that no shipped .pak provides means native code can ask ResourceBundle for a
resource that does not exist at runtime -- typically because a .grd was added
or moved upstream and electron_paks.gni was not updated. That normally ships
as a silently empty string; this check turns it into a build failure instead.
"""

import argparse
import glob
import os
import re
import sys


def load_id_names(gen_dir):
  """Maps resource id -> (name, header) using the grit-generated headers."""
  pattern = re.compile(r'#define (\S+) \(::ui::AllowlistedResource<(\d+)>')
  names = {}
  for header in glob.glob(
      os.path.join(gen_dir, '**', 'grit', '*.h'), recursive=True):
    try:
      with open(header, encoding='utf-8', errors='ignore') as f:
        text = f.read()
    except OSError:
      continue
    rel = os.path.relpath(header, gen_dir)
    for m in pattern.finditer(text):
      names.setdefault(int(m.group(2)), (m.group(1), rel))
  return names


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--allowlist', required=True,
                      help='Path to the generated resource allowlist')
  parser.add_argument('--pak', action='append', default=[],
                      help='A shipped .pak file (repeatable)')
  parser.add_argument('--gen-dir', required=True,
                      help='Root gen dir, used to map ids back to names')
  parser.add_argument('--grit-dir', required=True,
                      help='Path to //tools/grit for the data_pack module')
  parser.add_argument('--suppressions',
                      help='Optional file of resource names/ids to ignore')
  parser.add_argument('--stamp', required=True, help='Stamp file to touch')
  args = parser.parse_args()

  sys.path.insert(0, args.grit_dir)
  from grit.format import data_pack

  with open(args.allowlist) as f:
    allowlisted = {int(line) for line in f.read().split()}

  shipped = set()
  for pak in args.pak:
    shipped.update(data_pack.ReadDataPack(pak).resources.keys())

  suppressed_name_globs = []
  suppressed_headers = set()
  suppressed_ids = set()
  if args.suppressions:
    with open(args.suppressions) as f:
      for line in f:
        line = line.split('#', 1)[0].strip()
        if not line:
          continue
        if line.isdigit():
          suppressed_ids.add(int(line))
        elif line.startswith('header:'):
          suppressed_headers.add(line[len('header:'):])
        else:
          suppressed_name_globs.append(line)

  missing = sorted(allowlisted - shipped - suppressed_ids)
  if missing:
    import fnmatch
    names = load_id_names(args.gen_dir)

    def is_suppressed(resource_id):
      name, header = names.get(resource_id, ('', ''))
      if header in suppressed_headers:
        return True
      return any(fnmatch.fnmatch(name, glob) for glob in suppressed_name_globs)

    missing = [m for m in missing if not is_suppressed(m)]

  if missing:
    print('error: %d resource(s) are referenced by the electron binary but '
          'are not present in any shipped .pak file.' % len(missing),
          file=sys.stderr)
    print('Add the .grd that provides them to build/electron_paks.gni, or, if '
          'the referencing code is genuinely unreachable in Electron, add the '
          'resource name to the suppressions file.\n', file=sys.stderr)
    for resource_id in missing:
      name, header = names.get(resource_id, ('<unknown>', '<unknown header>'))
      print('  %s (id %d) from %s' % (name, resource_id, header),
            file=sys.stderr)
    return 1

  with open(args.stamp, 'w'):
    pass
  return 0


if __name__ == '__main__':
  sys.exit(main())
