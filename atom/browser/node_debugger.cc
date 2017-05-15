// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/node_debugger.h"

#include "base/command_line.h"
#include "libplatform/libplatform.h"

namespace atom {

NodeDebugger::NodeDebugger(node::Environment* env) : env_(env) {
}

NodeDebugger::~NodeDebugger() {
}

void NodeDebugger::Start() {
  auto inspector = env_->inspector_agent();
  if (inspector == nullptr)
    return;

  node::DebugOptions options;
  for (auto& arg : base::CommandLine::ForCurrentProcess()->argv())
    options.ParseOption(arg);

  if (options.inspector_enabled()) {
    // Use custom platform since the gin platform does not work correctly
    // with node's inspector agent
    platform_.reset(v8::platform::CreateDefaultPlatform());

    inspector->Start(platform_.get(), nullptr, options);
  }
}

}  // namespace atom
