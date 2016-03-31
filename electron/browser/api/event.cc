// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/event.h"

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/object_template_builder.h"

namespace mate {

namespace {

v8::Persistent<v8::ObjectTemplate> template_;

}  // namespace

Event::Event()
    : sender_(NULL),
      message_(NULL) {
}

Event::~Event() {
}

ObjectTemplateBuilder Event::GetObjectTemplateBuilder(v8::Isolate* isolate) {
  if (template_.IsEmpty())
    template_.Reset(isolate, ObjectTemplateBuilder(isolate)
        .SetMethod("preventDefault", &Event::PreventDefault)
        .SetMethod("sendReply", &Event::SendReply)
        .Build());

  return ObjectTemplateBuilder(
      isolate, v8::Local<v8::ObjectTemplate>::New(isolate, template_));
}

void Event::SetSenderAndMessage(content::WebContents* sender,
                                IPC::Message* message) {
  DCHECK(!sender_);
  DCHECK(!message_);
  sender_ = sender;
  message_ = message;

  Observe(sender);
}

void Event::WebContentsDestroyed() {
  sender_ = NULL;
  message_ = NULL;
}

void Event::PreventDefault(v8::Isolate* isolate) {
  GetWrapper(isolate)->Set(StringToV8(isolate, "defaultPrevented"),
                           v8::True(isolate));
}

bool Event::SendReply(const base::string16& json) {
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
