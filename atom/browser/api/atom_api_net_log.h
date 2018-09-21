// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_NET_LOG_H_
#define ATOM_BROWSER_API_ATOM_API_NET_LOG_H_

#include <memory>
#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "base/values.h"
#include "components/net_log/net_export_file_writer.h"

namespace atom {

namespace api {

class NetLog : public mate::TrackableObject<NetLog>,
               public net_log::NetExportFileWriter::StateObserver {
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

  // net_log::NetExportFileWriter::StateObserver implementation
  void OnNewState(const base::DictionaryValue& state) override;

 private:
  net_log::NetExportFileWriter* net_log_writer_;
  base::OnceClosure stop_callback_;
  std::unique_ptr<base::DictionaryValue> net_log_state_;

  DISALLOW_COPY_AND_ASSIGN(NetLog);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_NET_LOG_H_
