// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/uv_stdio_fix.h"

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <tuple>

void FixStdioStreams() {
  // libuv may mark stdin/stdout/stderr as close-on-exec, which interferes
  // with chromium's subprocess spawning. As a workaround, we detect if these
  // streams are closed on startup, and reopen them as /dev/null if necessary.
  // Otherwise, an unrelated file descriptor will be assigned as stdout/stderr
  // which may cause various errors when attempting to write to them.
  //
  // For details see https://github.com/libuv/libuv/issues/2062
  struct stat st;
  if (fstat(STDIN_FILENO, &st) < 0 && errno == EBADF)
    std::ignore = freopen("/dev/null", "r", stdin);
  if (fstat(STDOUT_FILENO, &st) < 0 && errno == EBADF)
    std::ignore = freopen("/dev/null", "w", stdout);
  if (fstat(STDERR_FILENO, &st) < 0 && errno == EBADF)
    std::ignore = freopen("/dev/null", "w", stderr);
}
