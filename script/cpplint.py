#!/usr/bin/env python

import fnmatch
import os
import subprocess
import sys

IGNORE_FILES = [
  'app/win/resource.h',
  'browser/atom_event_processing_window.h',
  'browser/atom_application_mac.h',
  'browser/atom_application_delegate_mac.h',
  'browser/native_window_mac.h',
  'browser/ui/atom_menu_controller_mac.h',
  'browser/ui/nsalert_synchronous_sheet_mac.h',
  'common/api/api_messages.cc',
  'common/api/api_messages.h',
  'common/atom_version.h',
]

SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))


def main():
  os.chdir(SOURCE_ROOT)
  files = list_files(['app', 'browser', 'common', 'renderer'],
                     ['*.cc', '*.h'])
  call_cpplint(list(set(files) - set(IGNORE_FILES)))


def list_files(directories, filters):
  matches = []
  for directory in directories:
    for root, dirs, filenames, in os.walk(directory):
      for f in filters:
        for filename in fnmatch.filter(filenames, f):
          matches.append(os.path.join(root, filename))
  return matches


def call_cpplint(files):
  cpplint = os.path.join(SOURCE_ROOT, 'vendor', 'depot_tools', 'cpplint.py')
  rules = '--filter=-build/header_guard,-build/include_what_you_use'
  subprocess.call([sys.executable, cpplint, rules] + files)


if __name__ == '__main__':
  sys.exit(main())
