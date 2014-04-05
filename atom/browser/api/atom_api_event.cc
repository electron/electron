// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_event.h"

#include "atom/common/api/api_messages.h"
#include "atom/common/v8/node_common.h"
#include "atom/common/v8/native_type_conversions.h"
#include "content/public/browser/web_contents.h"

namespace atom {

namespace api {

ScopedPersistent<v8::Function> Event::constructor_template_;

Event::Event()
    : sender_(NULL),
      message_(NULL),
      prevent_default_(false) {
}

Event::~Event() {
}

// static
v8::Handle<v8::Object> Event::CreateV8Object() {
  if (constructor_template_.IsEmpty()) {
    v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(v8::String::NewSymbol("Event"));

    NODE_SET_PROTOTYPE_METHOD(t, "preventDefault", PreventDefault);
    NODE_SET_PROTOTYPE_METHOD(t, "sendReply", SendReply);
    NODE_SET_PROTOTYPE_METHOD(t, "destroy", Destroy);

    constructor_template_.reset(t->GetFunction());
  }

  v8::Handle<v8::Function> t = constructor_template_.NewHandle(node_isolate);
  return t->NewInstance(0, NULL);
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

// static
void Event::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Event* event = new Event;
  event->Wrap(args.This());
}

// static
void Event::PreventDefault(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Event* event = Unwrap<Event>(args.This());
  if (event == NULL)
    return node::ThrowError("Event is already destroyed");

  event->prevent_default_ = true;
}

// static
void Event::SendReply(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Event* event = Unwrap<Event>(args.This());
  if (event == NULL)
    return node::ThrowError("Event is already destroyed");

  if (event->message_ == NULL || event->sender_ == NULL)
    return node::ThrowError("Can only send reply to synchronous events");

  string16 json = FromV8Value(args[0]);

  AtomViewHostMsg_Message_Sync::WriteReplyParams(event->message_, json);
  event->sender_->Send(event->message_);

  delete event;
}

// static
void Event::Destroy(const v8::FunctionCallbackInfo<v8::Value>& args) {
  delete Unwrap<Event>(args.This());
}

}  // namespace api

}  // namespace atom
