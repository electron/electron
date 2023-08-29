// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/process_util.h"

#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "gin/dictionary.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/node_includes.h"

namespace electron {

void EmitWarning(node::Environment* env,
                 const std::string& warning_msg,
                 const std::string& warning_type) {
  v8::HandleScope scope(env->isolate());
  gin::Dictionary process(env->isolate(), env->process_object());

  base::RepeatingCallback<void(base::StringPiece, base::StringPiece,
                               base::StringPiece)>
      emit_warning;
  process.Get("emitWarning", &emit_warning);

  emit_warning.Run(warning_msg, warning_type, "");
}

std::string GetProcessType() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->GetSwitchValueASCII(switches::kProcessType);
}

bool IsBrowserProcess() {
  static bool result = GetProcessType().empty();
  return result;
}

bool IsRendererProcess() {
  static bool result = GetProcessType() == switches::kRendererProcess;
  return result;
}

bool IsUtilityProcess() {
  static bool result = GetProcessType() == switches::kUtilityProcess;
  return result;
}

bool IsZygoteProcess() {
  static bool result = GetProcessType() == switches::kZygoteProcess;
  return result;
}

}  // namespace electron
