// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
#define ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_

#include <memory>
#include <vector>

#include "electron/buildflags/buildflags.h"

#include "base/compiler_specific.h"
#include "content/public/utility/content_utility_client.h"
#include "electron/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
#include "chrome/utility/printing_handler.h"
#endif

namespace atom {

class AtomContentUtilityClient : public content::ContentUtilityClient {
 public:
  AtomContentUtilityClient();
  ~AtomContentUtilityClient() override;

  void UtilityThreadStarted() override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void RegisterServices(StaticServiceMap* services) override;

 private:
#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
  std::unique_ptr<printing::PrintingHandler> printing_handler_;
#endif

  bool elevated_;

  DISALLOW_COPY_AND_ASSIGN(AtomContentUtilityClient);
};

}  // namespace atom

#endif  // ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
