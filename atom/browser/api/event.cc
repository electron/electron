// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/event.h"

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/object_template_builder.h"

namespace mate {

Event::Event()
    : sender_(NULL),
      message_(NULL),
      prevent_default_(false) {
}

Event::~Event() {
}

ObjectTemplateBuilder Event::GetObjectTemplateBuilder(v8::Isolate* isolate) {
  return ObjectTemplateBuilder(isolate)
      .SetMethod("preventDefault", &Event::PreventDefault)
      .SetMethod("sendReply", &Event::SendReply);
}

void Event::SetSenderAndMessage(content::WebContents* sender,
                                IPC::Message* message) {
  DCHECK(!sender_);
  DCHECK(!message_);
  sender_ = sender;
  message_ = message;

  Observe(sender);
}

void Event::WebContentsDestroyed(content::WebContents* web_contents) {
  sender_ = NULL;
  message_ = NULL;
}

void Event::PreventDefault() {
  prevent_default_ = true;
}

bool Event::SendReply(const string16& json) {
  if (message_ == NULL || sender_ == NULL)
    return false;

  AtomViewHostMsg_Message_Sync::WriteReplyParams(message_, json);
  return sender_->Send(message_);
}

// static
Handle<Event> Event::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new Event);
}

}  // namespace mate
