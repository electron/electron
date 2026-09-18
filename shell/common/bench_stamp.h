// Temporary startup-benchmark instrumentation (not for landing).
#ifndef ELECTRON_SHELL_COMMON_BENCH_STAMP_H_
#define ELECTRON_SHELL_COMMON_BENCH_STAMP_H_

#include "base/compiler_specific.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

namespace electron {

// Writes "BENCHTS:<name>:<tag>:<pid>:<realtime_ms>:<monotonic_ms>\n" to fd 1
// when ELECTRON_BENCH_STAMPS is set. Cheap enough to leave on every path.
inline void BenchStamp(const char* name, const char* tag = "") {
  static const bool enabled = getenv("ELECTRON_BENCH_STAMPS") != nullptr;
  if (!enabled)
    return;
  struct timespec rt, mt;
  clock_gettime(CLOCK_REALTIME, &rt);
  clock_gettime(CLOCK_MONOTONIC, &mt);
  char buf[256];
  int n = UNSAFE_BUFFERS(snprintf(
      buf, sizeof(buf), "BENCHTS:%s:%s:%d:%.3f:%.3f\n", name, tag,
      static_cast<int>(getpid()), rt.tv_sec * 1000.0 + rt.tv_nsec / 1e6,
      mt.tv_sec * 1000.0 + mt.tv_nsec / 1e6));
  if (n > 0) {
    ssize_t r = write(1, buf, static_cast<size_t>(n));
    (void)r;
  }
}

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_BENCH_STAMP_H_
