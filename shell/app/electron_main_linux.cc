// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <cstdlib>
#include <utility>

#include "base/at_exit.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "content/public/app/content_main.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/app/electron_main_delegate.h"  // NOLINT
#include "shell/app/node_main.h"
#include "shell/app/uv_stdio_fix.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/electron_constants.h"

int main(int argc, char* argv[]) {
  bool use_custom_v8_snapshot_file_name =
      electron::fuses::IsLoadV8SnapshotFromCustomPathEnabled();
  int new_argc = argc + (use_custom_v8_snapshot_file_name ? 1 : 0);
  char* new_argv[new_argc];
  for (int i = 0; i < argc; i++) {
    new_argv[i] = argv[i];
  }
  if (use_custom_v8_snapshot_file_name) {
    new_argv[new_argc - 1] = const_cast<char*>(
        "--custom-v8-snapshot-file-name=custom_v8_context_snapshot.bin");
  }

  FixStdioStreams();

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  char* indicator = getenv(electron::kRunAsNode);
  if (electron::fuses::IsRunAsNodeEnabled() && indicator &&
      indicator[0] != '\0') {
    base::i18n::InitializeICU();
    base::AtExitManager atexit_manager;
    return electron::NodeMain(new_argc, new_argv);
  }
#endif

  electron::ElectronMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  electron::ElectronCommandLine::Init(new_argc, new_argv);
  params.argc = new_argc;
  params.argv = const_cast<const char**>(new_argv);
  base::CommandLine::Init(params.argc, params.argv);
  // TODO(https://crbug.com/1176772): Remove when Chrome Linux is fully migrated
  // to Crashpad.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      ::switches::kEnableCrashpad);
  return content::ContentMain(std::move(params));
}
