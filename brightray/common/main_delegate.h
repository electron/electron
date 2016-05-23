// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_COMMON_MAIN_DELEGATE_H_
#define BRIGHTRAY_COMMON_MAIN_DELEGATE_H_

#include "base/macros.h"
#include "content/public/app/content_main_delegate.h"

namespace base {
class FilePath;
}

namespace ui {
class ResourceBundle;
}

namespace brightray {

class BrowserClient;
class ContentClient;

void InitializeResourceBundle(const std::string& locale);
base::FilePath GetResourcesPakFilePath();

class MainDelegate : public content::ContentMainDelegate {
 public:
  MainDelegate();
  ~MainDelegate();

 protected:
  // Subclasses can override this to provide their own ContentClient
  // implementation.
  virtual std::unique_ptr<ContentClient> CreateContentClient();

  // Subclasses can override this to provide their own BrowserClient
  // implementation.
  virtual std::unique_ptr<BrowserClient> CreateBrowserClient();

#if defined(OS_MACOSX)
  // Subclasses can override this to custom the paths of child process and
  // framework bundle.
  virtual void OverrideChildProcessPath();
  virtual void OverrideFrameworkBundlePath();
#endif

  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;

 private:
  content::ContentBrowserClient* CreateContentBrowserClient() override;

  std::unique_ptr<ContentClient> content_client_;
  std::unique_ptr<BrowserClient> browser_client_;

  DISALLOW_COPY_AND_ASSIGN(MainDelegate);
};

}  // namespace brightray
#endif
