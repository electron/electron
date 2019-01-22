// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_MAIN_DELEGATE_H_
#define ATOM_APP_ATOM_MAIN_DELEGATE_H_

#include <memory>
#include <string>

#include "content/public/app/content_main_delegate.h"
#include "content/public/common/content_client.h"

namespace atom {

void LoadResourceBundle(const std::string& locale);

class AtomMainDelegate : public content::ContentMainDelegate {
 public:
  AtomMainDelegate();
  ~AtomMainDelegate() override;

 protected:
  // content::ContentMainDelegate:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  void PreCreateMainMessageLoop() override;
  void PostEarlyInitialization(bool is_running_tests) override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;
  content::ContentUtilityClient* CreateContentUtilityClient() override;
  int RunProcess(
      const std::string& process_type,
      const content::MainFunctionParams& main_function_params) override;
#if defined(OS_MACOSX)
  bool ShouldSendMachPort(const std::string& process_type) override;
  bool DelaySandboxInitialization(const std::string& process_type) override;
#endif
  bool ShouldLockSchemeRegistry() override;
  bool ShouldCreateFeatureList() override;

 private:
#if defined(OS_MACOSX)
  void OverrideChildProcessPath();
  void OverrideFrameworkBundlePath();
  void SetUpBundleOverrides();
#endif

  std::unique_ptr<content::ContentBrowserClient> browser_client_;
  std::unique_ptr<content::ContentClient> content_client_;
  std::unique_ptr<content::ContentRendererClient> renderer_client_;
  std::unique_ptr<content::ContentUtilityClient> utility_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomMainDelegate);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_MAIN_DELEGATE_H_
