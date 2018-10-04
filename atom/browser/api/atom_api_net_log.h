// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_NET_LOG_H_
#define ATOM_BROWSER_API_ATOM_API_NET_LOG_H_

#include <list>
#include <memory>
#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "base/values.h"
#include "components/net_log/net_export_file_writer.h"
#include "native_mate/handle.h"

namespace atom {

class AtomBrowserContext;

namespace api {

class NetLog : public mate::TrackableObject<NetLog>,
               public net_log::NetExportFileWriter::StateObserver {
 public:
  static mate::Handle<NetLog> Create(v8::Isolate* isolate,
                                     AtomBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void StartLogging(mate::Arguments* args);
  std::string GetLoggingState() const;
  bool IsCurrentlyLogging() const;
  std::string GetCurrentlyLoggingPath() const;
  void StopLogging(mate::Arguments* args);

 protected:
  explicit NetLog(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~NetLog() override;

  // net_log::NetExportFileWriter::StateObserver implementation
  void OnNewState(const base::DictionaryValue& state) override;

 private:
  AtomBrowserContext* browser_context_;
  net_log::NetExportFileWriter* net_log_writer_;
  std::list<net_log::NetExportFileWriter::FilePathCallback>
      stop_callback_queue_;
  std::unique_ptr<base::DictionaryValue> net_log_state_;

  DISALLOW_COPY_AND_ASSIGN(NetLog);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_NET_LOG_H_
