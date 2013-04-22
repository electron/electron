// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/api/atom_renderer_bindings.h"

#include "base/logging.h"
#include "base/values.h"
#include "common/api/api_messages.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/v8_value_converter.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "vendor/node/src/node.h"

using content::RenderView;
using content::V8ValueConverter;
using WebKit::WebFrame;
using WebKit::WebView;

namespace atom {

namespace {

v8::Handle<v8::Object> GetProcessObject(v8::Handle<v8::Context> context) {
  v8::Handle<v8::Object> process =
      context->Global()->Get(v8::String::New("process"))->ToObject();
  DCHECK(!process.IsEmpty());

  return process;
}

RenderView* GetCurrentRenderView() {
  WebFrame* frame = WebFrame::frameForCurrentContext();
  DCHECK(frame);
  if (!frame)
    return NULL;

  WebView* view = frame->view();
  if (!view)
    return NULL;  // can happen during closing.

  RenderView* render_view = RenderView::FromWebView(view);
  DCHECK(render_view);
  return render_view;
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

void AtomRendererBindings::AddIPCBindings(WebFrame* frame) {
  v8::HandleScope handle_scope;

  v8::Handle<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope scope(context);

  v8::Handle<v8::Object> process = GetProcessObject(context);

  node::SetMethod(process, "send", Send);
}

// static
v8::Handle<v8::Value> AtomRendererBindings::Send(const v8::Arguments &args) {
  v8::HandleScope scope;

  RenderView* render_view = GetCurrentRenderView();

  // Convert Arguments to Array, so we can use V8ValueConverter to convert it
  // to ListValue.
  v8::Local<v8::Array> v8_args = v8::Array::New(args.Length());
  for (int i = 0; i < args.Length(); ++i)
    v8_args->Set(i, args[i]);

  scoped_ptr<V8ValueConverter> converter(V8ValueConverter::create());
  scoped_ptr<base::Value> arguments(
      converter->FromV8Value(v8_args, v8::Context::GetCurrent()));

  DCHECK(arguments && arguments->IsType(base::Value::TYPE_LIST));

  render_view->Send(new AtomViewHostMsg_Message(
      render_view->GetRoutingID(),
      *static_cast<base::ListValue*>(arguments.get())));

  return v8::Undefined();
}

}  // namespace atom
