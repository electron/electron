// Copyright (c) 2024 Notion, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_
#define SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_

#include "electron/shell/common/gin_helper/dictionary.h"
#include "electron/shell/common/gin_helper/event_emitter.h"
#include "electron/shell/common/gin_helper/event_emitter_caller.h"
#include "electron/shell/common/gin_helper/function_template.h"
#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"

namespace gin_helper {
// This object wraps the InvokeCallback so that if it gets GC'd by V8, we can
// still call the callback and send an error. Not doing so causes a Mojo DCHECK,
// since Mojo requires callbacks to be called before they are destroyed.
template <typename InvokeCallback>
class ReplyChannel : public gin::Wrappable<ReplyChannel<InvokeCallback>> {
 public:
  static gin::Handle<ReplyChannel> Create(v8::Isolate* isolate,
                                          InvokeCallback callback) {
    return gin::CreateHandle(isolate,
                             new ReplyChannel(isolate, std::move(callback)));
  }

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<ReplyChannel>::GetObjectTemplateBuilder(isolate)
        .SetMethod("sendReply", &ReplyChannel::SendReply);
  }
  const char* GetTypeName() override { return "ReplyChannel"; }

  void SendError(const std::string& msg) {
    // If there's no current context, it means we're shutting down, so we
    // don't need to send an event.
    if (!isolate_->GetCurrentContext().IsEmpty()) {
      v8::HandleScope scope(isolate_);
      auto message =
          gin::DataObjectBuilder(isolate_.get()).Set("error", msg).Build();
      SendReply(message);
    }
  }

 private:
  explicit ReplyChannel(v8::Isolate* isolate, InvokeCallback callback)
      : isolate_(isolate), callback_(std::move(callback)) {}
  ~ReplyChannel() override {
    if (callback_)
      SendError("reply was never sent");
  }

  bool SendReply(v8::Local<v8::Value> arg) {
    if (!callback_)
      return false;
    blink::CloneableMessage message;
    if (!gin::ConvertFromV8(isolate_.get(), arg, &message)) {
      return false;
    }

    std::move(callback_).Run(std::move(message));
    return true;
  }
  raw_ptr<v8::Isolate> isolate_;
  InvokeCallback callback_;
};

template <typename InvokeCallback>
gin::WrapperInfo ReplyChannel<InvokeCallback>::kWrapperInfo = {
    gin::kEmbedderNativeGin};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_
