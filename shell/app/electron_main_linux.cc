// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <cstdlib>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/strings/cstring_view.h"
#include "content/public/app/content_main.h"
#include "electron/fuses.h"
#include "shell/app/electron_main_delegate.h"  // NOLINT
#include "shell/app/node_main.h"
#include "shell/app/uv_stdio_fix.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/electron_constants.h"
#include "uv.h"

namespace {

[[nodiscard]] bool IsEnvSet(const base::cstring_view name) {
  const char* const indicator = getenv(name.c_str());
  return indicator && *indicator;
}

}  // namespace

int main(int argc, char* argv[]) {
  FixStdioStreams();

  argv = uv_setup_args(argc, argv);
  base::CommandLine::Init(argc, argv);
  electron::ElectronCommandLine::Init(argc, argv);

  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
    base::i18n::InitializeICU();
    base::AtExitManager atexit_manager;
    return electron::NodeMain();
  }

  electron::ElectronMainDelegate delegate;
  return content::ContentMain(content::ContentMainParams{&delegate});
}
