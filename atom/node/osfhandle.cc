// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "osfhandle.h"

#include <io.h>

namespace node {

int open_osfhandle(intptr_t osfhandle, int flags) {
  return _open_osfhandle(osfhandle, flags);
}

int close(int fd) {
  return _close(fd);
}

}  // namespace node
