// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_COMMON_MAIN_DELEGATE_H_
#define BRIGHTRAY_COMMON_MAIN_DELEGATE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/public/app/content_main_delegate.h"

namespace base {
class FilePath;
}

namespace ui {
class ResourceBundle;
}

namespace brightray {

void LoadResourceBundle(const std::string& locale);
void LoadCommonResources();

class MainDelegate : public content::ContentMainDelegate {
 public:
  MainDelegate();
  ~MainDelegate() override;

 protected:
#if defined(OS_MACOSX)
  // Subclasses can override this to custom the paths of child process and
  // framework bundle.
  virtual void OverrideChildProcessPath();
  virtual void OverrideFrameworkBundlePath();
#endif

  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MainDelegate);
};

}  // namespace brightray

#endif  // BRIGHTRAY_COMMON_MAIN_DELEGATE_H_
