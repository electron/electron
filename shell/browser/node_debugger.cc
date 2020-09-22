// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/node_debugger.h"

#include <memory>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "libplatform/libplatform.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron {

NodeDebugger::NodeDebugger(node::Environment* env) : env_(env) {}

NodeDebugger::~NodeDebugger() = default;

void NodeDebugger::Start() {
  auto* inspector = env_->inspector_agent();
  if (inspector == nullptr)
    return;

  // DebugOptions will already have been set by ProcessGlobalArgs,
  // so just pull the ones from there to pass to the InspectorAgent
  const auto debug_options = env_->options()->debug_options();
  if (inspector->Start("" /* path */, debug_options,
                       std::make_shared<node::ExclusiveAccess<node::HostPort>>(
                           debug_options.host_port),
                       true /* is_main */))
    DCHECK(inspector->IsListening());

  v8::HandleScope handle_scope(env_->isolate());
  node::profiler::StartProfilers(env_);

  if (inspector->options().break_node_first_line) {
    inspector->PauseOnNextJavascriptStatement("Break at bootstrap");
  }
}

void NodeDebugger::Stop() {
  auto* inspector = env_->inspector_agent();
  if (inspector && inspector->IsListening()) {
    inspector->WaitForDisconnect();
    inspector->Stop();
  }
}

}  // namespace electron
