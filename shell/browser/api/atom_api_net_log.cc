// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_net_log.h"

#include <utility>

#include "base/command_line.h"
#include "chrome/browser/browser_process.h"
#include "components/net_log/chrome_net_log.h"
#include "content/public/browser/storage_partition.h"
#include "electron/electron_version.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "net/url_request/url_request_context_getter.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/browser/net/system_network_context_manager.h"
#include "shell/common/native_mate_converters/callback.h"
#include "shell/common/native_mate_converters/file_path_converter.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace {

scoped_refptr<base::SequencedTaskRunner> CreateFileTaskRunner() {
  // The tasks posted to this sequenced task runner do synchronous File I/O for
  // checking paths and setting permissions on files.
  //
  // These operations can be skipped on shutdown since FileNetLogObserver's API
  // doesn't require things to have completed until notified of completion.
  return base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

base::File OpenFileForWriting(base::FilePath path) {
  return base::File(path,
                    base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
}

void ResolvePromiseWithNetError(util::Promise promise, int32_t error) {
  if (error == net::OK) {
    promise.Resolve();
  } else {
    promise.RejectWithErrorMessage(net::ErrorToString(error));
  }
}

}  // namespace

namespace api {

NetLog::NetLog(v8::Isolate* isolate, AtomBrowserContext* browser_context)
    : browser_context_(browser_context), weak_ptr_factory_(this) {
  Init(isolate);
  file_task_runner_ = CreateFileTaskRunner();
}

NetLog::~NetLog() = default;

v8::Local<v8::Promise> NetLog::StartLogging(mate::Arguments* args) {
  base::FilePath log_path;
  if (!args->GetNext(&log_path) || log_path.empty()) {
    args->ThrowError("The first parameter must be a valid string");
    return v8::Local<v8::Promise>();
  }

  if (net_log_exporter_) {
    args->ThrowError("There is already a net log running");
    return v8::Local<v8::Promise>();
  }

  pending_start_promise_ = base::make_optional<util::Promise>(isolate());
  v8::Local<v8::Promise> handle = pending_start_promise_->GetHandle();

  auto command_line_string =
      base::CommandLine::ForCurrentProcess()->GetCommandLineString();
  auto channel_string = std::string("Electron " ELECTRON_VERSION);
  base::Value custom_constants =
      base::Value::FromUniquePtrValue(net_log::GetPlatformConstantsForNetLog(
          command_line_string, channel_string));

  auto* network_context =
      content::BrowserContext::GetDefaultStoragePartition(browser_context_)
          ->GetNetworkContext();

  network_context->CreateNetLogExporter(mojo::MakeRequest(&net_log_exporter_));
  net_log_exporter_.set_connection_error_handler(
      base::BindOnce(&NetLog::OnConnectionError, base::Unretained(this)));

  // TODO(deepak1556): Provide more flexibility to this module
  // by allowing customizations on the capturing options.
  auto capture_mode = net::NetLogCaptureMode::kDefault;
  auto max_file_size = network::mojom::NetLogExporter::kUnlimitedFileSize;

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(OpenFileForWriting, log_path),
      base::BindOnce(&NetLog::StartNetLogAfterCreateFile,
                     weak_ptr_factory_.GetWeakPtr(), capture_mode,
                     max_file_size, std::move(custom_constants)));

  return handle;
}

void NetLog::StartNetLogAfterCreateFile(net::NetLogCaptureMode capture_mode,
                                        uint64_t max_file_size,
                                        base::Value custom_constants,
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

v8::Local<v8::Promise> NetLog::StopLogging(mate::Arguments* args) {
  util::Promise promise(isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (net_log_exporter_) {
    // Move the net_log_exporter_ into the callback to ensure that the mojo
    // pointer lives long enough to resolve the promise. Moving it into the
    // callback will cause the instance variable to become empty.
    net_log_exporter_->Stop(
        base::Value(base::Value::Type::DICTIONARY),
        base::BindOnce(
            [](network::mojom::NetLogExporterPtr, util::Promise promise,
               int32_t error) {
              ResolvePromiseWithNetError(std::move(promise), error);
            },
            std::move(net_log_exporter_), std::move(promise)));
  } else {
    promise.RejectWithErrorMessage("No net log in progress");
  }

  return handle;
}

// static
mate::Handle<NetLog> NetLog::Create(v8::Isolate* isolate,
                                    AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new NetLog(isolate, browser_context));
}

// static
void NetLog::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "NetLog"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("currentlyLogging", &NetLog::IsCurrentlyLogging)
      .SetMethod("startLogging", &NetLog::StartLogging)
      .SetMethod("stopLogging", &NetLog::StopLogging);
}

}  // namespace api

}  // namespace electron
