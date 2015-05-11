// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
#define ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_

#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/scoped_vector.h"
#include "content/public/utility/content_utility_client.h"
#include "ipc/ipc_platform_file.h"

namespace base {
class FilePath;
struct FileDescriptor;
}

class UtilityMessageHandler;

namespace atom {

class AtomContentUtilityClient : public content::ContentUtilityClient {
 public:
  AtomContentUtilityClient();
  ~AtomContentUtilityClient() override;

  void UtilityThreadStarted() override;
  bool OnMessageReceived(const IPC::Message& message) override;

  static void PreSandboxStartup();

  static void set_max_ipc_message_size_for_test(int64_t max_message_size) {
    max_ipc_message_size_ = max_message_size;
  }

 private:
  void OnStartupPing();

  typedef ScopedVector<UtilityMessageHandler> Handlers;
  Handlers handlers_;

  // Flag to enable whitelisting.
  bool filter_messages_;
  // A list of message_ids to filter.
  std::set<int> message_id_whitelist_;
  // Maximum IPC msg size (default to kMaximumMessageSize; override for testing)
  static int64_t max_ipc_message_size_;

  DISALLOW_COPY_AND_ASSIGN(AtomContentUtilityClient);
};

}  // namespace atom

#endif  // ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_ 
