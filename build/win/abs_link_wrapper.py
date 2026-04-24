#!/usr/bin/env python3
#
# Copyright (c) 2026 GitHub, Inc.
# Use of this source code is governed by the MIT license that can be
# found in the LICENSE file.

"""Wrapper for lld-link that resolves .obj paths in .rsp files to absolute.

Usage: abs_link_wrapper.py <lld-link args...>

The script finds the @rspfile argument, rewrites relative .obj/.res/.lib
paths inside it to absolute paths, then invokes lld-link with the
rewritten rsp file.
"""

import os
import subprocess
import sys


def _is_file_path(token):
    """Check if a token looks like a file path (not a linker flag)."""
    # Strip surrounding quotes
    t = token.strip('"')
    # Linker flags start with / or -
    if t.startswith('/') or t.startswith('-'):
        return False
    # File extensions we care about
    return t.endswith(('.obj', '.res', '.lib', '.a', '.o'))


def _resolve_rsp(rsp_path):
    """Rewrite relative file paths in the rsp file to absolute paths."""
    with open(rsp_path, 'r') as f:
        content = f.read()

    lines = []
    changed = False
    for line in content.splitlines():
        tokens = []
        for token in line.split():
            stripped = token.strip('"')
            if _is_file_path(token) and not os.path.isabs(stripped):
                abs_path = os.path.abspath(stripped)
                tokens.append('"' + abs_path + '"')
                changed = True
            else:
                tokens.append(token)
        lines.append(' '.join(tokens))

    if not changed:
        return rsp_path

    abs_rsp = rsp_path + '.abs'
    with open(abs_rsp, 'w') as f:
        f.write('\n'.join(lines))
    return abs_rsp


def main():
    args = []
    for arg in sys.argv[1:]:
        if arg.startswith('@'):
            rsp_path = arg[1:].strip('"')
            resolved = _resolve_rsp(rsp_path)
            args.append('@' + resolved)
        else:
            args.append(arg)

    return subprocess.call(args)


if __name__ == '__main__':
    sys.exit(main())
