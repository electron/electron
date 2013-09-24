// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/api/atom_api_renderer_ipc.h"

#include "base/values.h"
#include "common/api/api_messages.h"
#include "common/v8_conversions.h"
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

namespace api {

namespace {

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

// static
v8::Handle<v8::Value> RendererIPC::Send(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString())
    return node::ThrowTypeError("Bad argument");

  string16 channel = FromV8Value(args[0]);
  RenderView* render_view = GetCurrentRenderView();

  // Convert Arguments to Array, so we can use V8ValueConverter to convert it
  // to ListValue.
  v8::Local<v8::Array> v8_args = v8::Array::New(args.Length() - 1);
  for (int i = 0; i < args.Length() - 1; ++i)
    v8_args->Set(i, args[i + 1]);

  scoped_ptr<V8ValueConverter> converter(V8ValueConverter::create());
  scoped_ptr<base::Value> arguments(
      converter->FromV8Value(v8_args, v8::Context::GetCurrent()));

  DCHECK(arguments && arguments->IsType(base::Value::TYPE_LIST));

  bool success = render_view->Send(new AtomViewHostMsg_Message(
      render_view->GetRoutingID(),
      channel,
      *static_cast<base::ListValue*>(arguments.get())));

  if (!success)
    return node::ThrowError("Unable to send AtomViewHostMsg_Message");

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> RendererIPC::SendSync(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString())
    return node::ThrowTypeError("Bad argument");

  v8::Handle<v8::Context> context = v8::Context::GetCurrent();
  string16 channel = FromV8Value(args[0]);

  // Convert Arguments to Array, so we can use V8ValueConverter to convert it
  // to ListValue.
  v8::Local<v8::Array> v8_args = v8::Array::New(args.Length() - 1);
  for (int i = 0; i < args.Length() - 1; ++i)
    v8_args->Set(i, args[i + 1]);

  scoped_ptr<V8ValueConverter> converter(V8ValueConverter::create());
  scoped_ptr<base::Value> arguments(converter->FromV8Value(v8_args, context));

  DCHECK(arguments && arguments->IsType(base::Value::TYPE_LIST));

  RenderView* render_view = GetCurrentRenderView();

  string16 json;
  IPC::SyncMessage* message = new AtomViewHostMsg_Message_Sync(
      render_view->GetRoutingID(),
      channel,
      *static_cast<base::ListValue*>(arguments.get()),
      &json);
  // Enable the UI thread in browser to receive messages.
  message->EnableMessagePumping();
  bool success = render_view->Send(message);

  if (!success)
    return node::ThrowError("Unable to send AtomViewHostMsg_Message_Sync");

  return scope.Close(ToV8Value(json));
}

// static
void RendererIPC::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "send", Send);
  node::SetMethod(target, "sendSync", SendSync);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_renderer_ipc, atom::api::RendererIPC::Initialize)
