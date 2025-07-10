// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <optional>

#include "base/command_line.h"
#include "base/dcheck_is_on.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "content/browser/network_service_instance_impl.h"  // nogncheck
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_switches.h"
#include "shell/common/callback_util.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

#if DCHECK_IS_ON()
namespace {

class CallbackTestingHelper final {
 public:
  void HoldRepeatingCallback(const base::RepeatingClosure& callback) {
    repeating_callback_ = callback;
  }

  bool CopyHeldRepeatingCallback() {
    if (!repeating_callback_)
      return false;

    repeating_callback_copy_ = *repeating_callback_;
    return true;
  }

  bool InvokeHeldRepeatingCallback(v8::Isolate* isolate) {
    if (!repeating_callback_)
      return false;

    return InvokeRepeatingCallback(isolate, *repeating_callback_);
  }

  bool InvokeCopiedRepeatingCallback(v8::Isolate* isolate) {
    if (!repeating_callback_copy_)
      return false;

    return InvokeRepeatingCallback(isolate, *repeating_callback_copy_);
  }

  void HoldOnceCallback(base::OnceClosure callback) {
    once_callback_ = std::move(callback);
  }

  bool InvokeHeldOnceCallback(v8::Isolate* isolate) {
    if (!once_callback_)
      return false;

    base::OnceClosure callback = std::move(*once_callback_);
    once_callback_.reset();
    return InvokeOnceCallback(isolate, std::move(callback));
  }

  void ClearPrimaryHeldRepeatingCallback() { repeating_callback_.reset(); }

  int GetHeldRepeatingCallbackCount() const {
    return (repeating_callback_ ? 1 : 0) + (repeating_callback_copy_ ? 1 : 0);
  }

  void ClearAllHeldCallbacks() {
    repeating_callback_.reset();
    repeating_callback_copy_.reset();
    once_callback_.reset();
  }

 private:
  bool InvokeRepeatingCallback(v8::Isolate* isolate,
                               const base::RepeatingClosure& callback) {
    v8::TryCatch try_catch(isolate);
    callback.Run();
    if (try_catch.HasCaught()) {
      try_catch.Reset();
      return false;
    }

    return true;
  }

  bool InvokeOnceCallback(v8::Isolate* isolate, base::OnceClosure callback) {
    v8::TryCatch try_catch(isolate);
    std::move(callback).Run();
    if (try_catch.HasCaught()) {
      try_catch.Reset();
      return false;
    }

    return true;
  }

  std::optional<base::RepeatingClosure> repeating_callback_;
  std::optional<base::RepeatingClosure> repeating_callback_copy_;
  std::optional<base::OnceClosure> once_callback_;
};

CallbackTestingHelper& GetCallbackTestingHelper() {
  static base::NoDestructor<CallbackTestingHelper> helper;
  return *helper;
}

void Log(int severity, std::string text) {
  switch (severity) {
    case logging::LOGGING_VERBOSE:
      VLOG(1) << text;
      break;
    case logging::LOGGING_INFO:
      LOG(INFO) << text;
      break;
    case logging::LOGGING_WARNING:
      LOG(WARNING) << text;
      break;
    case logging::LOGGING_ERROR:
      LOG(ERROR) << text;
      break;
    case logging::LOGGING_FATAL:
      LOG(FATAL) << text;
      // break not needed here because LOG(FATAL) is [[noreturn]]
    default:
      LOG(ERROR) << "Unrecognized severity: " << severity;
      break;
  }
}

std::string GetLoggingDestination() {
  const auto* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->GetSwitchValueASCII(switches::kEnableLogging);
}

v8::Local<v8::Promise> SimulateNetworkServiceCrash(v8::Isolate* isolate) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  auto subscription = content::RegisterNetworkServiceProcessGoneHandler(
      electron::AdaptCallbackForRepeating(
          base::BindOnce([](gin_helper::Promise<void> promise,
                            bool crashed) { promise.Resolve(); },
                         std::move(promise))));
  content::RestartNetworkService();
  return handle;
}

void HoldRepeatingCallbackForTesting(const base::RepeatingClosure& callback) {
  GetCallbackTestingHelper().HoldRepeatingCallback(callback);
}

bool CopyHeldRepeatingCallbackForTesting() {
  return GetCallbackTestingHelper().CopyHeldRepeatingCallback();
}

bool InvokeHeldRepeatingCallbackForTesting(gin::Arguments* args) {
  return GetCallbackTestingHelper().InvokeHeldRepeatingCallback(
      args->isolate());
}

bool InvokeCopiedRepeatingCallbackForTesting(gin::Arguments* args) {
  return GetCallbackTestingHelper().InvokeCopiedRepeatingCallback(
      args->isolate());
}

void HoldOnceCallbackForTesting(base::OnceClosure callback) {
  GetCallbackTestingHelper().HoldOnceCallback(std::move(callback));
}

bool InvokeHeldOnceCallbackForTesting(gin::Arguments* args) {
  return GetCallbackTestingHelper().InvokeHeldOnceCallback(args->isolate());
}

void ClearPrimaryHeldRepeatingCallbackForTesting() {
  GetCallbackTestingHelper().ClearPrimaryHeldRepeatingCallback();
}

int GetHeldRepeatingCallbackCountForTesting() {
  return GetCallbackTestingHelper().GetHeldRepeatingCallbackCount();
}

void ClearHeldCallbacksForTesting() {
  GetCallbackTestingHelper().ClearAllHeldCallbacks();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("log", &Log);
  dict.SetMethod("getLoggingDestination", &GetLoggingDestination);
  dict.SetMethod("simulateNetworkServiceCrash", &SimulateNetworkServiceCrash);
  dict.SetMethod("holdRepeatingCallbackForTesting",
                 &HoldRepeatingCallbackForTesting);
  dict.SetMethod("copyHeldRepeatingCallbackForTesting",
                 &CopyHeldRepeatingCallbackForTesting);
  dict.SetMethod("invokeHeldRepeatingCallbackForTesting",
                 &InvokeHeldRepeatingCallbackForTesting);
  dict.SetMethod("invokeCopiedRepeatingCallbackForTesting",
                 &InvokeCopiedRepeatingCallbackForTesting);
  dict.SetMethod("clearPrimaryHeldRepeatingCallbackForTesting",
                 &ClearPrimaryHeldRepeatingCallbackForTesting);
  dict.SetMethod("getHeldRepeatingCallbackCountForTesting",
                 &GetHeldRepeatingCallbackCountForTesting);
  dict.SetMethod("holdOnceCallbackForTesting", &HoldOnceCallbackForTesting);
  dict.SetMethod("invokeHeldOnceCallbackForTesting",
                 &InvokeHeldOnceCallbackForTesting);
  dict.SetMethod("clearHeldCallbacksForTesting", &ClearHeldCallbacksForTesting);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_testing, Initialize)
#endif
