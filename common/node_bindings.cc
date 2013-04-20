// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/node_bindings.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "v8/include/v8.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"
#include "vendor/node/src/node_javascript.h"

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

  // Open node's error reporting system for browser process.
  node::g_standalone_mode = is_browser_;

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
  }
}

void NodeBindings::Load() {
  v8::HandleScope scope;
  v8::Context::Scope context_scope(node::g_context);
  node::Load(node::process);
}

void NodeBindings::BindTo(WebKit::WebFrame* frame) {
  v8::HandleScope handle_scope;

  v8::Handle<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope scope(context);

  // Erase security token.
  context->SetSecurityToken(node::g_context->GetSecurityToken());

  // Evaluate cefode.js.
  v8::Handle<v8::Script> script = node::CompileCefodeMainSource();
  v8::Local<v8::Value> result = script->Run();

  // Run the script of cefode.js.
  std::string script_path(GURL(frame->document().url()).path());
  v8::Handle<v8::Value> args[2] = {
    v8::Local<v8::Value>::New(node::process),
    v8::String::New(script_path.c_str(), script_path.size())
  };
  v8::Local<v8::Function>::Cast(result)->Call(context->Global(), 2, args);
}

}  // namespace atom
