// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_REMOTE_DEBUGGING_SERVER_H_
#define BROWSER_REMOTE_DEBUGGING_SERVER_H_

#include <string>

#include "base/basictypes.h"

namespace content {
class DevToolsHttpHandler;
}

namespace brightray {

class RemoteDebuggingServer {
 public:
  RemoteDebuggingServer(const std::string& ip, uint16 port);
  virtual ~RemoteDebuggingServer();

 private:
  content::DevToolsHttpHandler* devtools_http_handler_;

  DISALLOW_COPY_AND_ASSIGN(RemoteDebuggingServer);
};

}  // namespace brightray

#endif  // BROWSER_REMOTE_DEBUGGING_SERVER_H_
