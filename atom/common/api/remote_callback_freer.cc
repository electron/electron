// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/remote_callback_freer.h"

#include "atom/common/api/api_messages.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

// static
void RemoteCallbackFreer::BindTo(v8::Isolate* isolate,
                                 v8::Local<v8::Object> target,
                                 int object_id,
                                 content::WebContents* web_contents) {
  new RemoteCallbackFreer(isolate, target, object_id, web_contents);
}

RemoteCallbackFreer::RemoteCallbackFreer(v8::Isolate* isolate,
                                         v8::Local<v8::Object> target,
                                         int object_id,
                                         content::WebContents* web_contents)
    : ObjectLifeMonitor(isolate, target),
      content::WebContentsObserver(web_contents),
      web_contents_(web_contents),
      renderer_process_id_(GetRendererProcessID()),
      object_id_(object_id) {
}

RemoteCallbackFreer::~RemoteCallbackFreer() {
}

void RemoteCallbackFreer::RunDestructor() {
  if (!web_contents_)
    return;

  if (renderer_process_id_ == GetRendererProcessID()) {
    base::string16 channel =
        base::ASCIIToUTF16("ELECTRON_RENDERER_RELEASE_CALLBACK");
    base::ListValue args;
    args.AppendInteger(object_id_);
    Send(new AtomViewMsg_Message(routing_id(), channel, args));
  }
  web_contents_ = nullptr;
}

void RemoteCallbackFreer::RenderViewDeleted(content::RenderViewHost*) {
  if (!web_contents_)
    return;

  web_contents_ = nullptr;
  delete this;
}

int RemoteCallbackFreer::GetRendererProcessID() {
  if (!web_contents_)
    return -1;

  auto process = web_contents()->GetRenderProcessHost();
  if (!process)
    return -1;

  return process->GetID();
}

}  // namespace atom
