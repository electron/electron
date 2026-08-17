// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/preallocate_fd_table_linux.h"

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>

#include "base/files/scoped_file.h"
#include "base/posix/eintr_wrapper.h"

namespace electron {

namespace {

// Enough for launch and ordinary use; 8 KB of kernel memory per process.
constexpr int kPreallocatedFileDescriptors = 1024;

}  // namespace

void PreallocateFileDescriptorTable() {
  struct rlimit limit;
  if (getrlimit(RLIMIT_NOFILE, &limit) != 0)
    return;
  const int highest_fd =
      std::min<rlim_t>(kPreallocatedFileDescriptors, limit.rlim_cur) - 1;
  base::ScopedFD null_fd(HANDLE_EINTR(open("/dev/null", O_RDONLY | O_CLOEXEC)));
  if (!null_fd.is_valid() || highest_fd <= null_fd.get())
    return;
  // Duplicating onto a high descriptor number is what sizes the table.
  if (HANDLE_EINTR(dup3(null_fd.get(), highest_fd, O_CLOEXEC)) == highest_fd)
    close(highest_fd);
}

}  // namespace electron
