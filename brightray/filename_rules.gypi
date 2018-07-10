# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This gypi file defines the patterns used for determining whether a
# file is excluded from the build on a given platform.  It is
# included by common.gypi for chromium_code.

{
  'target_conditions': [
    ['OS!="win"', {
      'sources/': [ ['exclude', '_win(_unittest)?\\.(h|cc)$'],
                    ['exclude', '(^|/)win/'],
                    ['exclude', '(^|/)win_[^/]*\\.(h|cc)$'] ],
    }],
    ['OS!="mac"', {
      'sources/': [ ['exclude', '_(cocoa|mac)(_unittest)?\\.(h|cc|mm?)$'],
                    ['exclude', '(^|/)(cocoa|mac)/'] ],
    }],
    ['OS!="ios"', {
      'sources/': [ ['exclude', '_ios(_unittest)?\\.(h|cc|mm?)$'],
                    ['exclude', '(^|/)ios/'] ],
    }],
    ['OS!="mac" and OS!="ios"', {
      'sources/': [ ['exclude', '\\.mm?$' ] ],
    }],
    # Do not exclude the linux files on *BSD since most of them can be
    # shared at this point.
    # In case a file is not needed, it is going to be excluded later on.
    # TODO(evan): the above is not correct; we shouldn't build _linux
    # files on non-linux.
    ['OS!="linux" and OS!="openbsd" and OS!="freebsd"', {
      'sources/': [
        ['exclude', '(^|/)library_loaders/'],
        ['exclude', '_linux(_unittest)?\\.(h|cc)$'],
        ['exclude', '(^|/)linux_[^/]*\\.(h|cc)$'],
        ['exclude', '(^|/)linux/'],
        ['exclude', '_x11(_unittest)?\\.(h|cc)$'],
        ['exclude', '(^|/)x11_[^/]*\\.(h|cc)$'],
        ['exclude', '(^|/)x11/'],
      ],
    }],
    ['OS!="android"', {
      'sources/': [
        ['exclude', '_android(_unittest)?\\.cc$'],
        ['exclude', '(^|/)android/'],
      ],
    }],
    ['OS=="win"', {
      'sources/': [
        ['exclude', '_posix(_unittest)?\\.(h|cc)$'],
        ['exclude', '(^|/)posix/'],
      ],
    }],
    ['OS!="linux" and OS!="openbsd" and OS!="freebsd"', {
      'sources/': [
        ['exclude', '_xdg(_unittest)?\\.(h|cc)$'],
      ],
    }],
    ['OS!="linux" and OS!="openbsd" and OS!="freebsd"', {
      'sources/': [
        ['exclude', '_gtk(_browsertest|_unittest)?\\.(h|cc)$'],
        ['exclude', '(^|/)gtk/'],
        ['exclude', '(^|/)gtk_[^/]*\\.(h|cc)$'],
        ['exclude', '(^|/)libgtkui/'],
        ['exclude', '(^|/)x/'],
      ],
    }],
    ['OS=="mac"', {
      'sources/': [ ['exclude', '_aura(_browsertest|_unittest)?\\.(h|cc)$'],
                    ['exclude', '(^|/)aura/'],
                    ['exclude', '_views\\.(h|cc)$'],
                    ['exclude', '(^|/)views/'],
      ],
    }],
  ]
}
