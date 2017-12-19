#!/usr/bin/env python

import argparse
import fnmatch
import os
import sys

from lib.config import enable_verbose_mode
from lib.util import execute

IGNORE_FILES = [
  os.path.join('atom', 'browser', 'mac', 'atom_application.h'),
  os.path.join('atom', 'browser', 'mac', 'atom_application_delegate.h'),
  os.path.join('atom', 'browser', 'resources', 'win', 'resource.h'),
  os.path.join('atom', 'browser', 'ui', 'cocoa', 'atom_menu_controller.h'),
  os.path.join('atom', 'browser', 'ui', 'cocoa', 'atom_touch_bar.h'),
  os.path.join('atom', 'browser', 'ui', 'cocoa',
               'touch_bar_forward_declarations.h'),
  os.path.join('atom', 'browser', 'ui', 'cocoa', 'NSColor+Hex.h'),
  os.path.join('atom', 'browser', 'ui', 'cocoa', 'NSString+ANSI.h'),
  os.path.join('atom', 'common', 'api', 'api_messages.h'),
  os.path.join('atom', 'common', 'common_message_generator.cc'),
  os.path.join('atom', 'common', 'common_message_generator.h'),
  os.path.join('brightray', 'browser', 'mac',
               'bry_inspectable_web_contents_view.h'),
  os.path.join('brightray', 'browser', 'mac', 'event_dispatching_window.h'),
  os.path.join('brightray', 'browser', 'mac',
               'notification_center_delegate.h'),
  os.path.join('brightray', 'browser', 'win', 'notification_presenter_win7.h'),
  os.path.join('brightray', 'browser', 'win', 'win32_desktop_notifications',
               'common.h'),
  os.path.join('brightray', 'browser', 'win', 'win32_desktop_notifications',
               'desktop_notification_controller.cc'),
  os.path.join('brightray', 'browser', 'win', 'win32_desktop_notifications',
               'desktop_notification_controller.h'),
  os.path.join('brightray', 'browser', 'win', 'win32_desktop_notifications',
               'toast.h'),
  os.path.join('brightray', 'browser', 'win', 'win32_notification.h')
]

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def main():

  parser = argparse.ArgumentParser(
    description="Run cpplint on Electron's C++ files",
    formatter_class=argparse.RawTextHelpFormatter
  )
  parser.add_argument(
    '-c', '--only-changed',
    action='store_true',
    default=False,
    dest='only_changed',
    help='only run on changed files'
  )
  parser.add_argument(
    '-v', '--verbose',
    action='store_true',
    default=False,
    dest='verbose',
    help='show cpplint output'
  )
  args = parser.parse_args()

  if not os.path.isfile(cpplint_path()):
    print("[INFO] Skipping cpplint, dependencies has not been bootstrapped")
    return

  if args.only_changed:
    changed_files = get_changed_files()

  if args.verbose:
    enable_verbose_mode()

  ignore = set(IGNORE_FILES)

  os.chdir(SOURCE_ROOT)
  files = list_files('atom',
                     ['app', 'browser', 'common', 'renderer', 'utility'],
                     ['*.cc', '*.h'])
  files += list_files('brightray', ['browser', 'common'], ['*.cc', '*.h'])
  files -= ignore
  if args.only_changed:
    files &= changed_files
  call_cpplint(list(files))


def list_files(parent, directories, filters):
  matches = set()
  for directory in directories:
    for root, _, filenames, in os.walk(os.path.join(parent, directory)):
      for f in filters:
        for filename in fnmatch.filter(filenames, f):
          matches.add(os.path.join(root, filename))
  return matches


def get_changed_files():
  return set(execute(['git', 'diff', '--name-only']).splitlines())


def call_cpplint(files):
  if files:
    cpplint = cpplint_path()
    execute([sys.executable, cpplint] + files)


def cpplint_path():
  return os.path.join(SOURCE_ROOT, 'vendor', 'depot_tools', 'cpplint.py')


if __name__ == '__main__':
  sys.exit(main())
