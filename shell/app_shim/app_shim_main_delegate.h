// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_SHIM_APP_SHIM_MAIN_DELEGATE_H_
#define ELECTRON_SHELL_APP_SHIM_APP_SHIM_MAIN_DELEGATE_H_

#include <memory>
#include <string>
#include <variant>

#include "base/files/file_path.h"
#include "content/public/app/content_main_delegate.h"

namespace electron {

class AppShimController;

// The content delegate of an app shim process: a process that loads the
// framework of a host Electron app, connects to it, and hosts the NSWindows of
// the browser windows that the host app assigns to it.
class AppShimMainDelegate : public content::ContentMainDelegate {
 public:
  // `host_bundle_path` is the .app bundle of the host Electron app.
  explicit AppShimMainDelegate(const base::FilePath& host_bundle_path);
  ~AppShimMainDelegate() override;

  AppShimMainDelegate(const AppShimMainDelegate&) = delete;
  AppShimMainDelegate& operator=(const AppShimMainDelegate&) = delete;

 private:
  // content::ContentMainDelegate:
  std::optional<int> BasicStartupComplete() override;
  void PreSandboxStartup() override;
  std::variant<int, content::MainFunctionParams> RunProcess(
      const std::string& process_type,
      content::MainFunctionParams main_function_params) override;
  bool ShouldCreateFeatureList(InvokedIn invoked_in) override;
  bool ShouldInitializeMojo(InvokedIn invoked_in) override;
  bool ShouldInitializePerfetto(InvokedIn invoked_in) override;
  bool ShouldReconfigurePartitionAlloc() override;
  bool ShouldLoadV8Snapshot(const std::string& process_type) override;
  content::ContentClient* CreateContentClient() override;

  const base::FilePath host_bundle_path_;
  std::unique_ptr<content::ContentClient> content_client_;
  std::unique_ptr<AppShimController> controller_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_SHIM_APP_SHIM_MAIN_DELEGATE_H_
