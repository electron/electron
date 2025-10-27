// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/dcheck_is_on.h"
#include "base/logging.h"
#include "content/public/common/content_switches.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

#if DCHECK_IS_ON()
namespace {

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

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("log", &Log);
  dict.SetMethod("getLoggingDestination", &GetLoggingDestination);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_testing, Initialize)
#endif
