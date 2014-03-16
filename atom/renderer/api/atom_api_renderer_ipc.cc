// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_renderer_ipc.h"

#include "atom/common/api/api_messages.h"
#include "atom/common/v8/native_type_conversions.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/v8/node_common.h"

using content::RenderView;
using WebKit::WebFrame;
using WebKit::WebView;

namespace atom {

namespace api {

namespace {

RenderView* GetCurrentRenderView() {
  WebFrame* frame = WebFrame::frameForCurrentContext();
  if (!frame)
    return NULL;

  WebView* view = frame->view();
  if (!view)
    return NULL;  // can happen during closing.

  return RenderView::FromWebView(view);
}

}  // namespace

// static
void RendererIPC::Send(const v8::FunctionCallbackInfo<v8::Value>& args) {
  string16 channel;
  scoped_ptr<base::Value> arguments;
  if (!FromV8Arguments(args, &channel, &arguments))
    return node::ThrowTypeError("Bad argument");

  RenderView* render_view = GetCurrentRenderView();
  if (render_view == NULL)
    return;

  bool success = render_view->Send(new AtomViewHostMsg_Message(
      render_view->GetRoutingID(),
      channel,
      *static_cast<base::ListValue*>(arguments.get())));

  if (!success)
    return node::ThrowError("Unable to send AtomViewHostMsg_Message");
}

// static
void RendererIPC::SendSync(const v8::FunctionCallbackInfo<v8::Value>& args) {
  string16 channel;
  scoped_ptr<base::Value> arguments;
  if (!FromV8Arguments(args, &channel, &arguments))
    return node::ThrowTypeError("Bad argument");

  RenderView* render_view = GetCurrentRenderView();
  if (render_view == NULL)
    return;

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

  args.GetReturnValue().Set(ToV8Value(json));
}

// static
void RendererIPC::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope handle_scope(node_isolate);

  NODE_SET_METHOD(target, "send", Send);
  NODE_SET_METHOD(target, "sendSync", SendSync);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_renderer_ipc, atom::api::RendererIPC::Initialize)
