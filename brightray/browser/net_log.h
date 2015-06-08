// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BROWSER_NET_LOG_H_
#define BROWSER_NET_LOG_H_

#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "net/log/net_log.h"

namespace net {
class URLRequestContext;
}

namespace brightray {

class NetLog : public net::NetLog,
               public net::NetLog::ThreadSafeObserver {
 public:
  explicit NetLog(net::URLRequestContext* context);
  ~NetLog() override;

  void OnAddEntry(const net::NetLog::Entry& entry) override;

 private:
  bool added_events_;
  // We use raw pointer to prevent reference cycle.
  net::URLRequestContext* const context_;
  base::ScopedFILE log_file_;

  DISALLOW_COPY_AND_ASSIGN(NetLog);
};

}  // namespace brightray

#endif  // BROWSER_NET_LOG_H_
