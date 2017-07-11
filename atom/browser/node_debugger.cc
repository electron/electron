// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/node_debugger.h"

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "libplatform/libplatform.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

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
  for (auto& arg : base::CommandLine::ForCurrentProcess()->argv()) {
#if defined(OS_WIN)
    options.ParseOption("Electron", base::UTF16ToUTF8(arg));
#else
    options.ParseOption("Electron", arg);
#endif
  }

  if (options.inspector_enabled()) {
    // Use custom platform since the gin platform does not work correctly
    // with node's inspector agent
    platform_.reset(v8::platform::CreateDefaultPlatform());

    // Set process._debugWaitConnect if --inspect-brk was specified to stop
    // the debugger on the first line
    if (options.wait_for_connect()) {
      mate::Dictionary process(env_->isolate(), env_->process_object());
      process.Set("_debugWaitConnect", true);
    }

    inspector->Start(platform_.get(), nullptr, options);
  }
}

}  // namespace atom
