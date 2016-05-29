// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/remote_callback_freer.h"

#include "atom/common/api/api_messages.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

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
      object_id_(object_id) {
}

RemoteCallbackFreer::~RemoteCallbackFreer() {
}

void RemoteCallbackFreer::RunDestructor() {
  base::string16 channel =
      base::ASCIIToUTF16("ELECTRON_RENDERER_RELEASE_CALLBACK");
  base::ListValue args;
  args.AppendInteger(object_id_);
  Send(new AtomViewMsg_Message(routing_id(), false, channel, args));

  Observe(nullptr);
}

void RemoteCallbackFreer::RenderViewDeleted(content::RenderViewHost*) {
  delete this;
}

}  // namespace atom
