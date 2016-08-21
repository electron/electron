// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_sandboxed_renderer_client.h"

#include <string>

#include "atom_natives.h"  // NOLINT: This file is generated with js2c

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/api/atom_api_renderer_ipc.h"
#include "atom/renderer/atom_render_view_observer.h"
#include "base/command_line.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/render_view_observer.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace atom {

namespace {

const std::string kBindingKey = "binding";

class AtomSandboxedRenderFrameObserver : public content::RenderFrameObserver {
 public:
  AtomSandboxedRenderFrameObserver(content::RenderFrame* frame,
                                   AtomSandboxedRendererClient* renderer_client)
      : content::RenderFrameObserver(frame),
        render_frame_(frame),
        world_id_(-1),
        renderer_client_(renderer_client) {}

  // content::RenderFrameObserver:
  void DidClearWindowObject() override {
    // Make sure every page will get a script context created.
    render_frame_->GetWebFrame()->executeScript(
        blink::WebScriptSource("void 0"));
  }

  void DidCreateScriptContext(v8::Handle<v8::Context> context,
                              int extension_group,
                              int world_id) override {
    if (world_id_ != -1 && world_id_ != world_id)
      return;
    world_id_ = world_id;
    renderer_client_->DidCreateScriptContext(context, render_frame_);
  }

  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int world_id) override {
    if (world_id_ != world_id)
      return;
    renderer_client_->WillReleaseScriptContext(context, render_frame_);
  }

  void OnDestruct() override {
    delete this;
  }

 private:
  content::RenderFrame* render_frame_;
  int world_id_;
  AtomSandboxedRendererClient* renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomSandboxedRenderFrameObserver);
};

class AtomSandboxedRenderViewObserver : public AtomRenderViewObserver {
 public:
  AtomSandboxedRenderViewObserver(content::RenderView* render_view,
                                  AtomSandboxedRendererClient* renderer_client)
    : AtomRenderViewObserver(render_view, nullptr),
    renderer_client_(renderer_client) {
    }

 protected:
  void EmitIPCEvent(blink::WebFrame* frame,
                    const base::string16& channel,
                    const base::ListValue& args) override {
    if (!frame || frame->isWebRemoteFrame())
      return;

    auto isolate = blink::mainThreadIsolate();
    v8::HandleScope handle_scope(isolate);
    auto context = frame->mainWorldScriptContext();
    v8::Context::Scope context_scope(context);
    v8::Local<v8::Value> argv[] = {
      mate::ConvertToV8(isolate, channel),
      mate::ConvertToV8(isolate, args)
    };
    renderer_client_->InvokeBindingCallback(
        context,
        "onMessage",
        std::vector<v8::Local<v8::Value>>(argv, argv + 2));
  }

 private:
  AtomSandboxedRendererClient* renderer_client_;
  DISALLOW_COPY_AND_ASSIGN(AtomSandboxedRenderViewObserver);
};

}  // namespace


AtomSandboxedRendererClient::AtomSandboxedRendererClient() {
}

AtomSandboxedRendererClient::~AtomSandboxedRendererClient() {
}

void AtomSandboxedRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new AtomSandboxedRenderFrameObserver(render_frame, this);
}

void AtomSandboxedRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  new AtomSandboxedRenderViewObserver(render_view, this);
}

void AtomSandboxedRendererClient::DidCreateScriptContext(
    v8::Handle<v8::Context> context, content::RenderFrame* render_frame) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  std::string preload_script = command_line->GetSwitchValueASCII(
      switches::kPreloadScript);
  if (preload_script.empty())
    return;

  auto isolate = context->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  // Wrap the bundle into a function that receives the binding object and the
  // preload script path as arguments.
  std::string preload_bundle_native(node::preload_bundle_native,
      node::preload_bundle_native + sizeof(node::preload_bundle_native));
  std::stringstream ss;
  ss << "(function(binding, preloadPath) {\n";
  ss << preload_bundle_native << "\n";
  ss << "})";
  std::string preload_wrapper = ss.str();
  // Compile the wrapper and run it to get the function object
  auto script = v8::Script::Compile(
      mate::ConvertToV8(isolate, preload_wrapper)->ToString());
  auto func = v8::Handle<v8::Function>::Cast(
      script->Run(context).ToLocalChecked());
  // Create and initialize the binding object
  auto binding = v8::Object::New(isolate);
  api::Initialize(binding, v8::Null(isolate), context, nullptr);
  v8::Local<v8::Value> args[] = {
    binding,
    mate::ConvertToV8(isolate, preload_script)
  };
  // Execute the function with proper arguments
  ignore_result(func->Call(context, v8::Null(isolate), 2, args));
  // Store the bindingt privately for handling messages from the main process.
  auto binding_key = mate::ConvertToV8(isolate, kBindingKey)->ToString();
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  context->Global()->SetPrivate(context, private_binding_key, binding);
}

void AtomSandboxedRendererClient::WillReleaseScriptContext(
    v8::Handle<v8::Context> context, content::RenderFrame* render_frame) {
  auto isolate = context->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  InvokeBindingCallback(context, "onExit", std::vector<v8::Local<v8::Value>>());
}

void AtomSandboxedRendererClient::InvokeBindingCallback(
    v8::Handle<v8::Context> context,
    std::string callback_name,
    std::vector<v8::Handle<v8::Value>> args) {
  auto isolate = context->GetIsolate();
  auto binding_key = mate::ConvertToV8(isolate, kBindingKey)->ToString();
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  v8::Local<v8::Value> value;
  if (!global_object->GetPrivate(context, private_binding_key).ToLocal(&value))
    return;
  if (value.IsEmpty() || !value->IsObject())
    return;
  auto binding = value->ToObject();
  auto callback_key = mate::ConvertToV8(isolate, callback_name)->ToString();
  auto callback_value = binding->Get(callback_key);
  DCHECK(callback_value->IsFunction());  // set by sandboxed_renderer/init.js
  auto callback = v8::Handle<v8::Function>::Cast(callback_value);
  ignore_result(callback->Call(context, binding, args.size(), &args[0]));
}

}  // namespace atom
