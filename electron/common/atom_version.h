// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_VERSION_H
#define ELECTRON_VERSION_H

#define ELECTRON_MAJOR_VERSION 0
#define ELECTRON_MINOR_VERSION 37
#define ELECTRON_PATCH_VERSION 3

#define ELECTRON_VERSION_IS_RELEASE 1

#ifndef ELECTRON_TAG
# define ELECTRON_TAG ""
#endif

#ifndef ELECTRON_STRINGIFY
#define ELECTRON_STRINGIFY(n) ELECTRON_STRINGIFY_HELPER(n)
#define ELECTRON_STRINGIFY_HELPER(n) #n
#endif

#if ELECTRON_VERSION_IS_RELEASE
# define ELECTRON_VERSION_STRING  ELECTRON_STRINGIFY(ELECTRON_MAJOR_VERSION) "." \
                              ELECTRON_STRINGIFY(ELECTRON_MINOR_VERSION) "." \
                              ELECTRON_STRINGIFY(ELECTRON_PATCH_VERSION)     \
                              ELECTRON_TAG
#else
# define ELECTRON_VERSION_STRING  ELECTRON_STRINGIFY(ELECTRON_MAJOR_VERSION) "." \
                              ELECTRON_STRINGIFY(ELECTRON_MINOR_VERSION) "." \
                              ELECTRON_STRINGIFY(ELECTRON_PATCH_VERSION)     \
                              ELECTRON_TAG "-pre"
#endif

#define ELECTRON_VERSION "v" ELECTRON_VERSION_STRING


#define ELECTRON_VERSION_AT_LEAST(major, minor, patch) \
  (( (major) < ELECTRON_MAJOR_VERSION) \
  || ((major) == ELECTRON_MAJOR_VERSION && (minor) < ELECTRON_MINOR_VERSION) \
  || ((major) == ELECTRON_MAJOR_VERSION && (minor) == ELECTRON_MINOR_VERSION && (patch) <= ELECTRON_PATCH_VERSION))

#endif /* ELECTRON_VERSION_H */
