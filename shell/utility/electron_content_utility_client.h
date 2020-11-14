// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_UTILITY_ELECTRON_CONTENT_UTILITY_CLIENT_H_
#define SHELL_UTILITY_ELECTRON_CONTENT_UTILITY_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "content/public/utility/content_utility_client.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "printing/buildflags/buildflags.h"

namespace printing {
class PrintingHandler;
}

namespace mojo {
class ServiceFactory;
}  // namespace mojo

namespace electron {

class ElectronContentUtilityClient : public content::ContentUtilityClient {
 public:
  ElectronContentUtilityClient();
  ~ElectronContentUtilityClient() override;

  void ExposeInterfacesToBrowser(mojo::BinderMap* binders) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void RegisterMainThreadServices(mojo::ServiceFactory& services) override;
  void RegisterIOThreadServices(mojo::ServiceFactory& services) override;

 private:
#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && defined(OS_WIN)
  std::unique_ptr<printing::PrintingHandler> printing_handler_;
#endif

  // True if the utility process runs with elevated privileges.
  bool utility_process_running_elevated_;

  DISALLOW_COPY_AND_ASSIGN(ElectronContentUtilityClient);
};

}  // namespace electron

#endif  // SHELL_UTILITY_ELECTRON_CONTENT_UTILITY_CLIENT_H_
