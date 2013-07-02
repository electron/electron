#!/usr/bin/env python

import os

"""Prints the absolute path of the root of atom-shell's source tree.
"""


print os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
