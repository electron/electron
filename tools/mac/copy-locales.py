#!/usr/bin/env python
# Copyright (c) 2013 GitHub, Inc.
# Use of this source code is governed by the MIT license that can be
# found in the LICENSE file.

import errno
import optparse
import os
import shutil
import sys

def main(argv):
  parser = optparse.OptionParser()
  usage = 'usage: %s [options ...] src dest locale_list'
  parser.set_usage(usage.replace('%s', '%prog'))
  parser.add_option('-d', dest='dash_to_underscore', action="store_true",
                    default=False,
                    help='map "en-US" to "en" and "-" to "_" in locales')

  (options, arglist) = parser.parse_args(argv)

  if len(arglist) < 4:
    print 'ERROR: need src, dest and list of locales'
    return 1

  src = arglist[1]
  dest = arglist[2]
  locales = arglist[3:]

  for locale in locales:
    # For Cocoa to find the locale at runtime, it needs to use '_' instead
    # of '-' (http://crbug.com/20441).  Also, 'en-US' should be represented
    # simply as 'en' (http://crbug.com/19165, http://crbug.com/25578).
    dirname = locale
    if options.dash_to_underscore:
      if locale == 'en-US':
        dirname = 'en'
      else:
        dirname = locale.replace('-', '_')

    dirname = os.path.join(dest, dirname + '.lproj')
    safe_mkdir(dirname)
    shutil.copy2(os.path.join(src, locale + '.pak'),
                 os.path.join(dirname, 'locale.pak'))


def safe_mkdir(path):
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise


if __name__ == '__main__':
  sys.exit(main(sys.argv))
