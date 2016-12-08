// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_isolated_world.h"

#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"

namespace atom {

NodeBindings* AtomIsolatedWorld::node_bindings_ = nullptr;
node::Environment* AtomIsolatedWorld::env_ = nullptr;

AtomIsolatedWorld::AtomIsolatedWorld(NodeBindings* node_bindings) :
    v8::Extension("AtomIsolatedWorld", "native function SetupNode();") {
  node_bindings_ = node_bindings;
  env_ = nullptr;
}

AtomIsolatedWorld::~AtomIsolatedWorld() {
  node_bindings_ = nullptr;
  env_ = nullptr;
}

node::Environment* AtomIsolatedWorld::CreateEnvironment(
    content::RenderFrame* frame) {
  blink::WebScriptSource source("SetupNode()");
  frame->GetWebFrame()->executeScriptInIsolatedWorld(
      1,
      &source,
      1,
      1,
      nullptr);
  return env_;
}

v8::Local<v8::FunctionTemplate> AtomIsolatedWorld::GetNativeFunctionTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::String> name) {
  if (name->Equals(v8::String::NewFromUtf8(isolate, "SetupNode")))
    return v8::FunctionTemplate::New(isolate, SetupNode);
  return v8::Local<v8::FunctionTemplate>();
}

// static
void AtomIsolatedWorld::SetupNode(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  env_ = node_bindings_->CreateEnvironment(
      args.GetIsolate()->GetCurrentContext());
}

// static
AtomIsolatedWorld* AtomIsolatedWorld::Create(NodeBindings* node_bindings) {
  AtomIsolatedWorld* world = new AtomIsolatedWorld(node_bindings);
  content::RenderThread::Get()->RegisterExtension(world);
  return world;
}

}  // namespace atom
