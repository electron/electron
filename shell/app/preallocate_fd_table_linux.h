// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_PREALLOCATE_FD_TABLE_LINUX_H_
#define ELECTRON_SHELL_APP_PREALLOCATE_FD_TABLE_LINUX_H_

namespace electron {

// The kernel grows a process's file descriptor table on demand and, once the
// table is shared by threads, waits for an RCU grace period before freeing the
// old one (fs/file.c, expand_fdtable), which stalls whichever open()/socket()
// call happened to cross the boundary for 10-100 ms. Call this while the
// process is still single-threaded so the table is grown once, for free.
void PreallocateFileDescriptorTable();

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_PREALLOCATE_FD_TABLE_LINUX_H_
