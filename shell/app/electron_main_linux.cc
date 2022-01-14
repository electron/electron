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

namespace {

ALLOW_UNUSED_TYPE bool IsEnvSet(const char* name) {
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
}

}  // namespace

int main(int argc, char* argv[]) {
  FixStdioStreams();

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
    base::i18n::InitializeICU();
    base::AtExitManager atexit_manager;
    return electron::NodeMain(argc, argv);
  }
#endif

  electron::ElectronMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  electron::ElectronCommandLine::Init(argc, argv);
  params.argc = argc;
  params.argv = const_cast<const char**>(argv);
  base::CommandLine::Init(params.argc, params.argv);
  // TODO(https://crbug.com/1176772): Remove when Chrome Linux is fully migrated
  // to Crashpad.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      ::switches::kEnableCrashpad);
  return content::ContentMain(std::move(params));
}
