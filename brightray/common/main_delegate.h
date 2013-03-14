// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_COMMON_MAIN_DELEGATE_H_
#define BRIGHTRAY_COMMON_MAIN_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/app/content_main_delegate.h"

namespace brightray {

class ContentClient;

class MainDelegate : public content::ContentMainDelegate {
public:
  MainDelegate();
  ~MainDelegate();

private:
  static void InitializeResourceBundle();
#if defined(OS_MACOSX)
  static void OverrideChildProcessPath();
  static void OverrideFrameworkBundlePath();
#endif

  virtual bool BasicStartupComplete(int* exit_code) OVERRIDE;
  virtual void PreSandboxStartup() OVERRIDE;

  scoped_ptr<ContentClient> content_client_;

  DISALLOW_COPY_AND_ASSIGN(MainDelegate);
};

}
#endif
