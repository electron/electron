// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "common/main_delegate.h"

#include "browser/browser_client.h"
#include "common/content_client.h"

#include "base/command_line.h"
#include "content/public/common/content_switches.h"

namespace brightray {

MainDelegate::MainDelegate()
    : content_client_(new ContentClient) {
}

MainDelegate::~MainDelegate() {
}

bool MainDelegate::BasicStartupComplete(int* exit_code) {
  SetContentClient(content_client_.get());
  return false;
}

void MainDelegate::PreSandboxStartup() {
#if defined(OS_MACOSX)
  OverrideChildProcessPath();
  OverrideFrameworkBundlePath();
#endif
  InitializeResourceBundle();
}

}
