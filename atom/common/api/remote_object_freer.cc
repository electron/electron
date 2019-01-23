// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/remote_object_freer.h"

#include "atom/common/api/api_messages.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

using blink::WebLocalFrame;

namespace atom {

namespace {

content::RenderFrame* GetCurrentRenderFrame() {
  WebLocalFrame* frame = WebLocalFrame::FrameForCurrentContext();
  if (!frame)
    return nullptr;

  return content::RenderFrame::FromWebFrame(frame);
}

}  // namespace

// static
void RemoteObjectFreer::BindTo(v8::Isolate* isolate,
                               v8::Local<v8::Object> target,
                               const std::string& context_id,
                               int object_id) {
  new RemoteObjectFreer(isolate, target, context_id, object_id);
}

RemoteObjectFreer::RemoteObjectFreer(v8::Isolate* isolate,
                                     v8::Local<v8::Object> target,
                                     const std::string& context_id,
                                     int object_id)
    : ObjectLifeMonitor(isolate, target),
      context_id_(context_id),
      object_id_(object_id),
      routing_id_(MSG_ROUTING_NONE) {
  content::RenderFrame* render_frame = GetCurrentRenderFrame();
  if (render_frame) {
    routing_id_ = render_frame->GetRoutingID();
  }
}

RemoteObjectFreer::~RemoteObjectFreer() {}

void RemoteObjectFreer::RunDestructor() {
  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(routing_id_);
  if (!render_frame)
    return;

  auto* channel = "ELECTRON_BROWSER_DEREFERENCE";
  base::ListValue args;
  args.AppendString(context_id_);
  args.AppendInteger(object_id_);
  render_frame->Send(new AtomFrameHostMsg_Message(render_frame->GetRoutingID(),
                                                  true, channel, args));
}

}  // namespace atom
