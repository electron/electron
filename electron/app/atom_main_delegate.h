// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_APP_ELECTRON_MAIN_DELEGATE_H_
#define ELECTRON_APP_ELECTRON_MAIN_DELEGATE_H_

#include "brightray/common/main_delegate.h"
#include "brightray/common/content_client.h"

namespace electron {

class AtomMainDelegate : public brightray::MainDelegate {
 public:
  AtomMainDelegate();
  ~AtomMainDelegate();

 protected:
  // content::ContentMainDelegate:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;
  content::ContentUtilityClient* CreateContentUtilityClient() override;

  // brightray::MainDelegate:
  scoped_ptr<brightray::ContentClient> CreateContentClient() override;
#if defined(OS_MACOSX)
  void OverrideChildProcessPath() override;
  void OverrideFrameworkBundlePath() override;
#endif

 private:
  brightray::ContentClient content_client_;
  scoped_ptr<content::ContentBrowserClient> browser_client_;
  scoped_ptr<content::ContentRendererClient> renderer_client_;
  scoped_ptr<content::ContentUtilityClient> utility_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomMainDelegate);
};

}  // namespace electron

#endif  // ELECTRON_APP_ELECTRON_MAIN_DELEGATE_H_
