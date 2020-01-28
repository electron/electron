// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/event.h"

#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/object_template_builder.h"
#include "shell/common/native_mate_converters/string16_converter.h"
#include "shell/common/native_mate_converters/value_converter.h"

namespace mate {

Event::Event(v8::Isolate* isolate) {
  Init(isolate);
}

Event::~Event() {}

void Event::SetSenderAndMessage(content::RenderFrameHost* sender,
                                base::Optional<MessageSyncCallback> callback) {
  DCHECK(!sender_);
  DCHECK(!callback_);
  sender_ = sender;
  callback_ = std::move(callback);

  Observe(content::WebContents::FromRenderFrameHost(sender));
}

void Event::RenderFrameDeleted(content::RenderFrameHost* rfh) {
  if (sender_ != rfh)
    return;
  sender_ = nullptr;
  callback_.reset();
}

void Event::RenderFrameHostChanged(content::RenderFrameHost* old_rfh,
                                   content::RenderFrameHost* new_rfh) {
  if (sender_ && sender_ == old_rfh)
    sender_ = new_rfh;
}

void Event::FrameDeleted(content::RenderFrameHost* rfh) {
  if (sender_ != rfh)
    return;
  sender_ = nullptr;
  callback_.reset();
}

void Event::PreventDefault(v8::Isolate* isolate) {
  GetWrapper()
      ->Set(isolate->GetCurrentContext(),
            StringToV8(isolate, "defaultPrevented"), v8::True(isolate))
      .Check();
}

bool Event::SendReply(const base::Value& result) {
  if (!callback_ || sender_ == nullptr)
    return false;

  base::ListValue ret;
  ret.GetList().push_back(result.Clone());

  std::move(*callback_).Run(std::move(ret));
  callback_.reset();
  return true;
}

// static
Handle<Event> Event::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new Event(isolate));
}

// static
void Event::BuildPrototype(v8::Isolate* isolate,
                           v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Event"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("preventDefault", &Event::PreventDefault)
      .SetMethod("sendReply", &Event::SendReply);
}

}  // namespace mate
