// Copyright (c) 2022 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_browser_main_parts.h"

#include "base/command_line.h"
#include "base/environment.h"

namespace electron {

void ElectronBrowserMainParts::SetDesktopStartupId() {
  // TODO(clavin): this was removed upstream in
  // https://chromium-review.googlesource.com/c/chromium/src/+/6819616 but it's
  // not clear if that was intentional.
  auto const env = base::Environment::Create();
  auto* const command_line = base::CommandLine::ForCurrentProcess();
  if (std::optional<std::string> desktop_startup_id =
          env->GetVar("DESKTOP_STARTUP_ID"))
    command_line->AppendSwitchASCII("desktop-startup-id", *desktop_startup_id);
}

}  // namespace electron
