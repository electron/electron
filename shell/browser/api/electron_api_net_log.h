// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_NET_LOG_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_NET_LOG_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/log/net_log_capture_mode.h"
#include "services/network/public/mojom/net_log.mojom.h"
#include "shell/common/gin_helper/promise.h"

namespace base {
class FilePath;
class TaskRunner;
}  // namespace base

namespace gin {
class Arguments;

template <typename T>
class Handle;
}  // namespace gin

namespace electron {

class ElectronBrowserContext;

namespace api {

// The code is referenced from the net_log::NetExportFileWriter class.
class NetLog final : public gin::Wrappable<NetLog> {
 public:
  static gin::Handle<NetLog> Create(v8::Isolate* isolate,
                                    ElectronBrowserContext* browser_context);

  v8::Local<v8::Promise> StartLogging(base::FilePath log_path,
                                      gin::Arguments* args);
  v8::Local<v8::Promise> StopLogging(gin::Arguments* args);
  bool IsCurrentlyLogging() const;

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  NetLog(const NetLog&) = delete;
  NetLog& operator=(const NetLog&) = delete;

 protected:
  explicit NetLog(v8::Isolate* isolate,
                  ElectronBrowserContext* browser_context);
  ~NetLog() override;

  void OnConnectionError();

  void StartNetLogAfterCreateFile(net::NetLogCaptureMode capture_mode,
                                  uint64_t max_file_size,
                                  base::Value::Dict custom_constants,
                                  base::File output_file);
  void NetLogStarted(int32_t error);

 private:
  raw_ptr<ElectronBrowserContext> browser_context_;

  mojo::Remote<network::mojom::NetLogExporter> net_log_exporter_;

  std::optional<gin_helper::Promise<void>> pending_start_promise_;

  scoped_refptr<base::TaskRunner> file_task_runner_;

  base::WeakPtrFactory<NetLog> weak_ptr_factory_{this};
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_NET_LOG_H_
