// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
#define SHELL_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "content/public/utility/content_utility_client.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "printing/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
#include "chrome/utility/printing_handler.h"
#endif

namespace electron {

class AtomContentUtilityClient : public content::ContentUtilityClient {
 public:
  AtomContentUtilityClient();
  ~AtomContentUtilityClient() override;

  void ExposeInterfacesToBrowser(mojo::BinderMap* binders) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  mojo::ServiceFactory* GetMainThreadServiceFactory() override;
  mojo::ServiceFactory* GetIOThreadServiceFactory() override;

 private:
#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
  std::unique_ptr<printing::PrintingHandler> printing_handler_;
#endif

  // True if the utility process runs with elevated privileges.
  bool utility_process_running_elevated_;

  DISALLOW_COPY_AND_ASSIGN(AtomContentUtilityClient);
};

}  // namespace electron

#endif  // SHELL_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
