// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_ELECTRON_PREFETCH_MAC_H_
#define ELECTRON_SHELL_APP_ELECTRON_PREFETCH_MAC_H_

namespace electron {

// On a cold start (the framework binary is not in the page cache), reads the
// framework binary, libffmpeg and the framework's large resources sequentially
// on a background thread, at low disk priority. The main thread and the child
// processes then find those pages cached instead of faulting them in one small
// read at a time. Only this architecture's slice of a universal binary is
// read. Does nothing if the framework is already resident or memory is short.
// Must be called from the browser process, before ContentMain.
void PrefetchFrameworkFilesIfCold();

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_ELECTRON_PREFETCH_MAC_H_
