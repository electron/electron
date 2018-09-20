// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
#define ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "content/public/utility/content_utility_client.h"

class UtilityMessageHandler;

namespace atom {

class AtomContentUtilityClient : public content::ContentUtilityClient {
 public:
  AtomContentUtilityClient();
  ~AtomContentUtilityClient() override;

  bool OnMessageReceived(const IPC::Message& message) override;
  void RegisterServices(StaticServiceMap* services) override;

 private:
#if defined(OS_WIN)
  typedef std::vector<std::unique_ptr<UtilityMessageHandler>> Handlers;
  Handlers handlers_;
#endif

  DISALLOW_COPY_AND_ASSIGN(AtomContentUtilityClient);
};

}  // namespace atom

#endif  // ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
