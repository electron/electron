#!/usr/bin/env python3

import subprocess
import os

# Find the nearest tag to the current HEAD.
# This is equivalent to our old logic of "use a value in package.json" for the
# following reasons:
#
# 1. Whenever we updated the package.json we ALSO pushed a tag with the same
#    version.
# 2. Whenever we _reverted_ a bump all we actually did was push a commit that
#    deleted the tag and changed the version number back.
#
# The only difference in the "git describe" technique is that technically a
# commit can "change" its version number if a tag is created / removed
# retroactively.  i.e. the first time a commit is pushed it will be 1.2.3
# and after the tag is made rebuilding the same commit will result in it being
# 1.2.4.

try:
  output = subprocess.check_output(
      ['git', 'describe', '--tags', '--abbrev=0'],
      cwd=os.path.abspath(os.path.join(os.path.dirname(__file__), '..')),
      stderr=subprocess.PIPE,
      universal_newlines=True)
  version = output.strip().replace('v', '')
  print(version)
except Exception:
  # When there is error we print a null version string instead of throwing an
  # exception, this is because for linux/bsd packages and some vendor builds
  # electron is built from a source code tarball and there is no git information
  # there.
  print('0.0.0-no-git-tag-found')
