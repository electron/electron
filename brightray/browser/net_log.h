// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_NET_LOG_H_
#define BRIGHTRAY_BROWSER_NET_LOG_H_

#include "base/files/scoped_file.h"
#include "net/log/net_log.h"
#include "net/log/file_net_log_observer.h"
#include "net/url_request/url_request_context.h"

namespace brightray {

class NetLog : public net::NetLog {
 public:
  NetLog();
  ~NetLog() override;

  void StartLogging(net::URLRequestContext* url_request_context);

 private:
  base::ScopedFILE log_file_;
  net::FileNetLogObserver write_to_file_observer_;

  DISALLOW_COPY_AND_ASSIGN(NetLog);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_NET_LOG_H_
