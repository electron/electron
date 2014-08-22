#!/usr/bin/env python

import fnmatch
import os
import sys

from lib.util import execute

IGNORE_FILES = [
  os.path.join('atom', 'app', 'atom_main.cc'),
  os.path.join('atom', 'browser', 'mac', 'atom_application.h'),
  os.path.join('atom', 'browser', 'mac', 'atom_application_delegate.h'),
  os.path.join('atom', 'browser', 'native_window_mac.h'),
  os.path.join('atom', 'browser', 'resources', 'win', 'resource.h'),
  os.path.join('atom', 'browser', 'ui', 'cocoa', 'event_processing_window.h'),
  os.path.join('atom', 'browser', 'ui', 'cocoa', 'atom_menu_controller.h'),
  os.path.join('atom', 'browser', 'ui', 'gtk', 'gtk_custom_menu.cc'),
  os.path.join('atom', 'browser', 'ui', 'gtk', 'gtk_custom_menu_item.cc'),
  os.path.join('atom', 'common', 'api', 'api_messages.h'),
  os.path.join('atom', 'common', 'api', 'atom_extensions.h'),
  os.path.join('atom', 'common', 'atom_version.h'),
  os.path.join('atom', 'common', 'common_message_generator.cc'),
  os.path.join('atom', 'common', 'common_message_generator.h'),
  os.path.join('atom', 'common', 'swap_or_assign.h'),
]

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():
  os.chdir(SOURCE_ROOT)
  files = list_files(['app', 'browser', 'common', 'renderer'],
                     ['*.cc', '*.h'])
  call_cpplint(list(set(files) - set(IGNORE_FILES)))


def list_files(directories, filters):
  matches = []
  for directory in directories:
    for root, _, filenames, in os.walk(os.path.join('atom', directory)):
      for f in filters:
        for filename in fnmatch.filter(filenames, f):
          matches.append(os.path.join(root, filename))
  return matches


def call_cpplint(files):
  cpplint = os.path.join(SOURCE_ROOT, 'vendor', 'depot_tools', 'cpplint.py')
  execute([sys.executable, cpplint] + files)


if __name__ == '__main__':
  sys.exit(main())
