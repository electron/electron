// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/remote_callback_freer.h"

#include "atom/common/api/api_messages.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

// static
void RemoteCallbackFreer::BindTo(v8::Isolate* isolate,
                                 v8::Local<v8::Object> target,
                                 int frame_id,
                                 const std::string& context_id,
                                 int object_id,
                                 content::WebContents* web_contents) {
  new RemoteCallbackFreer(isolate, target, frame_id, context_id, object_id,
                          web_contents);
}

RemoteCallbackFreer::RemoteCallbackFreer(v8::Isolate* isolate,
                                         v8::Local<v8::Object> target,
                                         int frame_id,
                                         const std::string& context_id,
                                         int object_id,
                                         content::WebContents* web_contents)
    : ObjectLifeMonitor(isolate, target),
      content::WebContentsObserver(web_contents),
      frame_id_(frame_id),
      context_id_(context_id),
      object_id_(object_id) {}

RemoteCallbackFreer::~RemoteCallbackFreer() {}

void RemoteCallbackFreer::RunDestructor() {
  auto* channel = "ELECTRON_RENDERER_RELEASE_CALLBACK";
  base::ListValue args;
  int32_t sender_id = 0;
  args.AppendString(context_id_);
  args.AppendInteger(object_id_);
  auto frames = web_contents()->GetAllFrames();
  auto iter = std::find_if(frames.begin(), frames.end(), [this](auto* f) {
    return f->GetRoutingID() == frame_id_;
  });

  if (iter != frames.end() && (*iter)->IsRenderFrameLive()) {
    (*iter)->Send(new AtomFrameMsg_Message(frame_id_, true /* internal */,
                                           false /* send_to_all */, channel,
                                           args, sender_id));
  }

  Observe(nullptr);
}

void RemoteCallbackFreer::RenderViewDeleted(content::RenderViewHost*) {
  delete this;
}

}  // namespace atom
