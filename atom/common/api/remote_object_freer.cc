// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/remote_object_freer.h"

#include "atom/common/api/api_messages.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

using blink::WebLocalFrame;
using blink::WebView;

namespace atom {

namespace {

content::RenderView* GetCurrentRenderView() {
  WebLocalFrame* frame = WebLocalFrame::frameForCurrentContext();
  if (!frame)
    return nullptr;

  WebView* view = frame->view();
  if (!view)
    return nullptr;  // can happen during closing.

  return content::RenderView::FromWebView(view);
}

}  // namespace

// static
void RemoteObjectFreer::BindTo(
    v8::Isolate* isolate, v8::Local<v8::Object> target, int object_id) {
  new RemoteObjectFreer(isolate, target, object_id);
}

RemoteObjectFreer::RemoteObjectFreer(
    v8::Isolate* isolate, v8::Local<v8::Object> target, int object_id)
    : ObjectLifeMonitor(isolate, target),
      object_id_(object_id),
      routing_id_(0) {
  content::RenderView* render_view = GetCurrentRenderView();
  if (render_view) {
    routing_id_ = render_view->GetRoutingID();
  }
}

RemoteObjectFreer::~RemoteObjectFreer() {
}

void RemoteObjectFreer::RunDestructor() {
  content::RenderView* render_view =
      content::RenderView::FromRoutingID(routing_id_);
  if (!render_view)
    return;

  base::string16 channel = base::ASCIIToUTF16("ipc-message");
  base::ListValue args;
  args.AppendString("ELECTRON_BROWSER_DEREFERENCE");
  args.AppendInteger(object_id_);
  render_view->Send(
      new AtomViewHostMsg_Message(render_view->GetRoutingID(), channel, args));
}

}  // namespace atom
