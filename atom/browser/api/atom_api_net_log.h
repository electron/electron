// Copyright (c) 2018 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_NET_LOG_H_
#define ATOM_BROWSER_API_ATOM_API_NET_LOG_H_

#include <string>
#include "brightray/browser/net_log.h"
#include "native_mate/wrappable.h"

namespace atom {

namespace api {

class NetLog : public mate::Wrappable<NetLog> {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void StartLogging(mate::Arguments* args);
  bool IsCurrentlyLogging();
  std::string GetCurrentlyLoggingPath();
  void StopLogging(mate::Arguments* args);

 protected:
  explicit NetLog(v8::Isolate* isolate);
  ~NetLog() override;

 private:
  brightray::NetLog* net_log_;

  DISALLOW_COPY_AND_ASSIGN(NetLog);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_NET_LOG_H_
