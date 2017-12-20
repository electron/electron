#!/usr/bin/env python

import argparse
import os
import sys

from lib.config import enable_verbose_mode
from lib.util import execute

IGNORE_FILES = set(os.path.join(*components) for components in [
  ['atom', 'browser', 'mac', 'atom_application.h'],
  ['atom', 'browser', 'mac', 'atom_application_delegate.h'],
  ['atom', 'browser', 'resources', 'win', 'resource.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'atom_menu_controller.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'atom_touch_bar.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'touch_bar_forward_declarations.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'NSColor+Hex.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'NSString+ANSI.h'],
  ['atom', 'common', 'api', 'api_messages.h'],
  ['atom', 'common', 'common_message_generator.cc'],
  ['atom', 'common', 'common_message_generator.h'],
  ['atom', 'node', 'osfhandle.cc'],
  ['brightray', 'browser', 'mac', 'bry_inspectable_web_contents_view.h'],
  ['brightray', 'browser', 'mac', 'event_dispatching_window.h'],
  ['brightray', 'browser', 'mac', 'notification_center_delegate.h'],
  ['brightray', 'browser', 'win', 'notification_presenter_win7.h'],
  ['brightray', 'browser', 'win', 'win32_desktop_notifications', 'common.h'],
  ['brightray', 'browser', 'win', 'win32_desktop_notifications',
   'desktop_notification_controller.cc'],
  ['brightray', 'browser', 'win', 'win32_desktop_notifications',
   'desktop_notification_controller.h'],
  ['brightray', 'browser', 'win', 'win32_desktop_notifications', 'toast.h'],
  ['brightray', 'browser', 'win', 'win32_notification.h']
])

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

  if args.verbose:
    enable_verbose_mode()

  os.chdir(SOURCE_ROOT)
  files = find_files(['atom', 'brightray'], is_cpp_file)
  files -= IGNORE_FILES
  if args.only_changed:
    files &= find_changed_files()
  call_cpplint(files)


def find_files(roots, test):
  matches = set()
  for root in roots:
    for parent, _, children, in os.walk(root):
      for child in children:
        filename = os.path.join(parent, child)
        if test(filename):
          matches.add(filename)
  return matches


def is_cpp_file(filename):
  return filename.endswith('.cc') or filename.endswith('.h')


def find_changed_files():
  return set(execute(['git', 'diff', '--name-only']).splitlines())


def call_cpplint(files):
  if files:
    cpplint = cpplint_path()
    execute([sys.executable, cpplint] + list(files))


def cpplint_path():
  return os.path.join(SOURCE_ROOT, 'vendor', 'depot_tools', 'cpplint.py')


if __name__ == '__main__':
  sys.exit(main())
