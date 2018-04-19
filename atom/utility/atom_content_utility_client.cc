// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/utility/atom_content_utility_client.h"

#if defined(OS_WIN)
#include "chrome/utility/printing_handler_win.h"
#endif

namespace atom {

AtomContentUtilityClient::AtomContentUtilityClient() {
#if defined(OS_WIN)
  handlers_.push_back(std::make_unique<printing::PrintingHandlerWin>());
#endif
}

AtomContentUtilityClient::~AtomContentUtilityClient() {}

bool AtomContentUtilityClient::OnMessageReceived(const IPC::Message& message) {
#if defined(OS_WIN)
  for (const auto& handler : handlers_) {
    if (handler->OnMessageReceived(message))
      return true;
  }
#endif

  return false;
}

}  // namespace atom
