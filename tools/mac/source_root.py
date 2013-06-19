#!/usr/bin/env python

import os

"""Prints the absolute path of the root of atom-shell's source tree.
"""


relative_source_root = os.path.join(__file__, '..', '..', '..')
print os.path.abspath(relative_source_root)
