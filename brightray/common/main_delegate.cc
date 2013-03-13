// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "main_delegate.h"

#include "browser_client.h"
#include "base/command_line.h"
#include "content/public/common/content_switches.h"

namespace brightray {

MainDelegate::MainDelegate() {
}

MainDelegate::~MainDelegate() {
}

void MainDelegate::PreSandboxStartup() {
  // FIXME: We don't currently support running sandboxed.
  CommandLine::ForCurrentProcess()->AppendSwitch(switches::kNoSandbox);

#if defined(OS_MACOSX)
  OverrideChildProcessPath();
  OverrideFrameworkBundlePath();
#endif
}

}
