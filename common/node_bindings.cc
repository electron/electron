// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/node_bindings.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "v8/include/v8.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

NodeBindings::NodeBindings(bool is_browser)
    : is_browser_(is_browser) {
}

NodeBindings::~NodeBindings() {
}

void NodeBindings::Initialize() {
  CommandLine::StringVector str_argv = CommandLine::ForCurrentProcess()->argv();

  // Convert string vector to char* array.
  std::vector<char*> argv(str_argv.size(), NULL);
  for (size_t i = 0; i < str_argv.size(); ++i)
    argv[i] = const_cast<char*>(str_argv[i].c_str());

  // Init node.
  node::Init(argv.size(), &argv[0]);
  v8::V8::Initialize();

  // Load node.js.
  v8::HandleScope scope;
  node::g_context = v8::Context::New();
  {
    v8::Context::Scope context_scope(node::g_context);
    v8::Handle<v8::Object> process = node::SetupProcessObject(
        argv.size(), &argv[0]);

    // Tell node.js we are in browser or renderer.
    v8::Handle<v8::String> type =
        is_browser_ ? v8::String::New("browser") : v8::String::New("renderer");
    process->Set(v8::String::New("__atom_type"), type);

    node::Load(process);
  }
}

}  // namespace atom
