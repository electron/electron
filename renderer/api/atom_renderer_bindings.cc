// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/api/atom_renderer_bindings.h"

#include <vector>

#include "base/logging.h"
#include "base/values.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/v8_value_converter.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "vendor/node/src/node.h"

using content::RenderView;
using content::V8ValueConverter;
using WebKit::WebFrame;

namespace atom {

namespace {

v8::Handle<v8::Object> GetProcessObject(v8::Handle<v8::Context> context) {
  v8::Handle<v8::Object> process =
      context->Global()->Get(v8::String::New("process"))->ToObject();
  DCHECK(!process.IsEmpty());

  return process;
}

}  // namespace

AtomRendererBindings::AtomRendererBindings(RenderView* render_view)
    : render_view_(render_view) {
}

AtomRendererBindings::~AtomRendererBindings() {
}

void AtomRendererBindings::BindToFrame(WebFrame* frame) {
  v8::HandleScope handle_scope;

  v8::Handle<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope scope(context);

  AtomBindings::BindTo(GetProcessObject(context));
}

void AtomRendererBindings::OnRendererMessage(const std::string& channel,
                                             const base::ListValue& args) {
  if (!render_view_->GetWebView())
    return;

  v8::HandleScope scope;

  v8::Local<v8::Context> context =
      render_view_->GetWebView()->mainFrame()->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Object> process = GetProcessObject(context);
  scoped_ptr<V8ValueConverter> converter(V8ValueConverter::create());

  std::vector<v8::Handle<v8::Value>> arguments;
  arguments.reserve(1 + args.GetSize());
  arguments.push_back(v8::String::New(channel.c_str(), channel.size()));

  for (size_t i = 0; i < args.GetSize(); i++) {
    const base::Value* value;
    if (args.Get(i, &value))
      arguments.push_back(converter->ToV8Value(value, context));
  }

  node::MakeCallback(process, "emit", arguments.size(), &arguments[0]);
}

}  // namespace atom
