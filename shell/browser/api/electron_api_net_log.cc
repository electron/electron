// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_net_log.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "chrome/browser/browser_process.h"
#include "components/net_log/chrome_net_log.h"
#include "content/public/browser/storage_partition.h"
#include "electron/electron_version.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "net/log/net_log_capture_mode.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/net/system_network_context_manager.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"

namespace gin {

template <>
struct Converter<net::NetLogCaptureMode> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     net::NetLogCaptureMode* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "default")
      *out = net::NetLogCaptureMode::kDefault;
    else if (type == "includeSensitive")
      *out = net::NetLogCaptureMode::kIncludeSensitive;
    else if (type == "everything")
      *out = net::NetLogCaptureMode::kEverything;
    else
      return false;
    return true;
  }
};

}  // namespace gin

namespace electron {

namespace {

scoped_refptr<base::SequencedTaskRunner> CreateFileTaskRunner() {
  // The tasks posted to this sequenced task runner do synchronous File I/O for
  // checking paths and setting permissions on files.
  //
  // These operations can be skipped on shutdown since FileNetLogObserver's API
  // doesn't require things to have completed until notified of completion.
  return base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

base::File OpenFileForWriting(base::FilePath path) {
  return base::File(path,
                    base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
}

void ResolvePromiseWithNetError(gin_helper::Promise<void> promise,
                                int32_t error) {
  if (error == net::OK) {
    promise.Resolve();
  } else {
    promise.RejectWithErrorMessage(net::ErrorToString(error));
  }
}

}  // namespace

namespace api {

gin::WrapperInfo NetLog::kWrapperInfo = {gin::kEmbedderNativeGin};

NetLog::NetLog(v8::Isolate* isolate, ElectronBrowserContext* browser_context)
    : browser_context_(browser_context) {
  file_task_runner_ = CreateFileTaskRunner();
}

NetLog::~NetLog() = default;

v8::Local<v8::Promise> NetLog::StartLogging(base::FilePath log_path,
                                            gin::Arguments* args) {
  if (log_path.empty()) {
    args->ThrowTypeError("The first parameter must be a valid string");
    return v8::Local<v8::Promise>();
  }

  net::NetLogCaptureMode capture_mode = net::NetLogCaptureMode::kDefault;
  uint64_t max_file_size = network::mojom::NetLogExporter::kUnlimitedFileSize;

  gin_helper::Dictionary dict;
  if (args->GetNext(&dict)) {
    v8::Local<v8::Value> capture_mode_v8;
    if (dict.Get("captureMode", &capture_mode_v8)) {
      if (!gin::ConvertFromV8(args->isolate(), capture_mode_v8,
                              &capture_mode)) {
        args->ThrowTypeError("Invalid value for captureMode");
        return v8::Local<v8::Promise>();
      }
    }
    v8::Local<v8::Value> max_file_size_v8;
    if (dict.Get("maxFileSize", &max_file_size_v8)) {
      if (!gin::ConvertFromV8(args->isolate(), max_file_size_v8,
                              &max_file_size)) {
        args->ThrowTypeError("Invalid value for maxFileSize");
        return v8::Local<v8::Promise>();
      }
    }
  }

  if (net_log_exporter_) {
    args->ThrowTypeError("There is already a net log running");
    return v8::Local<v8::Promise>();
  }

  pending_start_promise_ =
      std::make_optional<gin_helper::Promise<void>>(args->isolate());
  v8::Local<v8::Promise> handle = pending_start_promise_->GetHandle();

  auto command_line_string =
      base::CommandLine::ForCurrentProcess()->GetCommandLineString();
  auto channel_string = std::string("Electron " ELECTRON_VERSION);
  base::Value::Dict custom_constants = net_log::GetPlatformConstantsForNetLog(
      command_line_string, channel_string);

  auto* network_context =
      browser_context_->GetDefaultStoragePartition()->GetNetworkContext();

  network_context->CreateNetLogExporter(
      net_log_exporter_.BindNewPipeAndPassReceiver());
  net_log_exporter_.set_disconnect_handler(
      base::BindOnce(&NetLog::OnConnectionError, base::Unretained(this)));

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(OpenFileForWriting, log_path),
      base::BindOnce(&NetLog::StartNetLogAfterCreateFile,
                     weak_ptr_factory_.GetWeakPtr(), capture_mode,
                     max_file_size, std::move(custom_constants)));

  return handle;
}

void NetLog::StartNetLogAfterCreateFile(net::NetLogCaptureMode capture_mode,
                                        uint64_t max_file_size,
                                        base::Value::Dict custom_constants,
                                        base::File output_file) {
  if (!net_log_exporter_) {
    // Theoretically the mojo pipe could have been closed by the time we get
    // here via the connection error handler. If so, the promise has already
    // been resolved.
    return;
  }
  DCHECK(pending_start_promise_);
  if (!output_file.IsValid()) {
    std::move(*pending_start_promise_)
        .RejectWithErrorMessage(
            base::File::ErrorToString(output_file.error_details()));
    net_log_exporter_.reset();
    return;
  }
  net_log_exporter_->Start(
      std::move(output_file), std::move(custom_constants), capture_mode,
      max_file_size,
      base::BindOnce(&NetLog::NetLogStarted, base::Unretained(this)));
}

void NetLog::NetLogStarted(int32_t error) {
  DCHECK(pending_start_promise_);
  ResolvePromiseWithNetError(std::move(*pending_start_promise_), error);
}

void NetLog::OnConnectionError() {
  net_log_exporter_.reset();
  if (pending_start_promise_) {
    std::move(*pending_start_promise_)
        .RejectWithErrorMessage("Failed to start net log exporter");
  }
}

bool NetLog::IsCurrentlyLogging() const {
  return !!net_log_exporter_;
}

v8::Local<v8::Promise> NetLog::StopLogging(gin::Arguments* args) {
  gin_helper::Promise<void> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (net_log_exporter_) {
    // Move the net_log_exporter_ into the callback to ensure that the mojo
    // pointer lives long enough to resolve the promise. Moving it into the
    // callback will cause the instance variable to become empty.
    net_log_exporter_->Stop(
        base::Value::Dict(),
        base::BindOnce(
            [](mojo::Remote<network::mojom::NetLogExporter>,
               gin_helper::Promise<void> promise, int32_t error) {
              ResolvePromiseWithNetError(std::move(promise), error);
            },
            std::move(net_log_exporter_), std::move(promise)));
  } else {
    promise.RejectWithErrorMessage("No net log in progress");
  }

  return handle;
}

gin::ObjectTemplateBuilder NetLog::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<NetLog>::GetObjectTemplateBuilder(isolate)
      .SetProperty("currentlyLogging", &NetLog::IsCurrentlyLogging)
      .SetMethod("startLogging", &NetLog::StartLogging)
      .SetMethod("stopLogging", &NetLog::StopLogging);
}

const char* NetLog::GetTypeName() {
  return "NetLog";
}

// static
gin::Handle<NetLog> NetLog::Create(v8::Isolate* isolate,
                                   ElectronBrowserContext* browser_context) {
  return gin::CreateHandle(isolate, new NetLog(isolate, browser_context));
}

}  // namespace api

}  // namespace electron
