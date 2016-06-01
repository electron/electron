// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_MAIN_DELEGATE_H_
#define ATOM_APP_ATOM_MAIN_DELEGATE_H_

#include <string>

#include "brightray/common/main_delegate.h"
#include "brightray/common/content_client.h"

namespace atom {

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
#if defined(OS_MACOSX)
  int RunProcess(
      const std::string& process_type,
      const content::MainFunctionParams& main_function_params) override;
  bool ShouldSendMachPort(const std::string& process_type) override;
  bool DelaySandboxInitialization(const std::string& process_type) override;
#endif

  // brightray::MainDelegate:
  std::unique_ptr<brightray::ContentClient> CreateContentClient() override;
#if defined(OS_MACOSX)
  void OverrideChildProcessPath() override;
  void OverrideFrameworkBundlePath() override;
#endif

 private:
#if defined(OS_MACOSX)
  void SetUpBundleOverrides();
#endif

  brightray::ContentClient content_client_;
  std::unique_ptr<content::ContentBrowserClient> browser_client_;
  std::unique_ptr<content::ContentRendererClient> renderer_client_;
  std::unique_ptr<content::ContentUtilityClient> utility_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomMainDelegate);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_MAIN_DELEGATE_H_
