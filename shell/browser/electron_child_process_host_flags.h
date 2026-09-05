// Copyright (c) 2026 Microsoft GmbH. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_CHILD_PROCESS_HOST_FLAGS_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_CHILD_PROCESS_HOST_FLAGS_H_

#include "build/build_config.h"
#include "content/public/browser/child_process_host.h"

namespace electron {

// Flags for Electron-specific child processes to resolve the appropriate
// helper executable via ElectronBrowserClient::GetChildProcessSuffix().
enum class ElectronChildProcessHostFlags {
#if BUILDFLAG(IS_MAC)
  // Starts a child process with macOS entitlements that disable library
  // validation and allow unsigned executable memory. This allows the process
  // to load third-party libraries not signed by the same Team ID as the main
  // executable.
  kChildProcessHelperPlugin =
      content::ChildProcessHost::CHILD_EMBEDDER_FIRST + 1,
#endif  // BUILDFLAG(IS_MAC)
};

#if BUILDFLAG(IS_MAC)
// Helper app name suffix for the plugin child process. This must match the
// corresponding entry in Electron's BUILD.gn electron_plugin_helper_params.
inline constexpr const char kElectronMacHelperSuffixPlugin[] = " (Plugin)";
#endif  // BUILDFLAG(IS_MAC)

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_CHILD_PROCESS_HOST_FLAGS_H_
