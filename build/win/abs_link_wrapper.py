#!/usr/bin/env python3
#
# Copyright (c) 2026 GitHub, Inc.
# Use of this source code is governed by the MIT license that can be
# found in the LICENSE file.

"""Wrapper for lld-link that resolves relative paths to absolute.

Usage: abs_link_wrapper.py <lld-link> <args...>

The first argument is the real linker executable. The script resolves
relative .obj/.rlib/.res/.lib paths in both @rspfile contents and direct
command-line arguments to absolute paths, then invokes the real linker.
"""

import os
import subprocess
import sys


def _is_file_path(token):
    """Check if a token looks like a relative file path (not a flag or bare lib name)."""
    # Strip surrounding quotes
    t = token.strip('"')
    # Linker flags start with / or -
    if t.startswith('/') or t.startswith('-'):
        return False
    # Must contain a directory separator to be a relative path.
    # Bare names like "advapi32.lib" are system libraries resolved via
    # -libpath: or /winsysroot and must not be turned into absolute paths.
    if '/' not in t and '\\' not in t:
        return False
    # File extensions we care about
    return t.endswith(('.obj', '.res', '.lib', '.a', '.o', '.rlib'))


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


def _resolve_arg(arg):
    """Resolve a single command-line argument if it's a relative file path."""
    stripped = arg.strip('"')
    if _is_file_path(arg) and not os.path.isabs(stripped):
        return os.path.abspath(stripped)
    return arg


def main():
    args = []
    for arg in sys.argv[1:]:
        if arg.startswith('@'):
            rsp_path = arg[1:].strip('"')
            resolved = _resolve_rsp(rsp_path)
            args.append('@' + resolved)
        else:
            args.append(_resolve_arg(arg))

    return subprocess.call(args)


if __name__ == '__main__':
    sys.exit(main())
