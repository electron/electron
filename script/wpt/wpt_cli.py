#!/usr/bin/env python3
"""Run the `wpt` command of a web-platform-tests checkout with the `electron`
wptrunner product available.

  wpt_cli.py <wpt root> <wpt arguments>

wptrunner finds out-of-tree products through the `wptrunner.products` entry
point group, which Python reads from the package metadata it finds on
sys.path. product/ holds the product next to such metadata
(wptrunner_electron-0.dist-info), so putting that directory on sys.path is all
it takes. Nothing is installed into the Python environment, which for `pinned`
is one that vpython manages.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
  wpt_root = os.path.abspath(sys.argv[1])
  # Like running <wpt root>/wpt: its directory goes where this script's is.
  # This directory must not be on sys.path, it has a wpt.py of its own.
  sys.path[:] = [path for path in sys.path if path != HERE]
  sys.path[:0] = [wpt_root, os.path.join(HERE, 'product')]
  sys.argv = [os.path.join(wpt_root, 'wpt')] + sys.argv[2:]
  # Only importable once sys.path points into the checkout.
  # pylint: disable=import-outside-toplevel
  from tools.wpt import wpt
  wpt.main()


# wptrunner starts its workers with the multiprocessing "spawn" method, which
# imports this file again in every worker (with sys.path copied from here).
if __name__ == '__main__':
  main()
