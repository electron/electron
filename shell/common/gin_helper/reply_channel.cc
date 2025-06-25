// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/reply_channel.h"

#include "base/debug/stack_trace.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/blink_converter.h"

namespace gin_helper::internal {

// static
using InvokeCallback = electron::mojom::ElectronApiIPC::InvokeCallback;
gin::Handle<ReplyChannel> ReplyChannel::Create(v8::Isolate* isolate,
                                               InvokeCallback callback) {
  return gin::CreateHandle(isolate, new ReplyChannel(std::move(callback)));
}

gin::ObjectTemplateBuilder ReplyChannel::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<ReplyChannel>::GetObjectTemplateBuilder(isolate)
      .SetMethod("sendReply", &ReplyChannel::SendReply);
}

const char* ReplyChannel::GetTypeName() {
  return "ReplyChannel";
}

ReplyChannel::ReplyChannel(InvokeCallback callback)
    : callback_(std::move(callback)) {}

ReplyChannel::~ReplyChannel() {
  if (callback_)
    SendError("reply was never sent");
}

void ReplyChannel::SendError(const std::string& msg) {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  // If there's no current context, it means we're shutting down, so we
  // don't need to send an event.
  if (!isolate->GetCurrentContext().IsEmpty()) {
    v8::HandleScope scope(isolate);
    auto message = gin::DataObjectBuilder(isolate).Set("error", msg).Build();
    SendReply(isolate, message);
  }
}

bool ReplyChannel::SendReply(v8::Isolate* isolate, v8::Local<v8::Value> arg) {
  if (!callback_)
    return false;
  blink::CloneableMessage message;
  if (!gin::ConvertFromV8(isolate, arg, &message)) {
    return false;
  }

  std::move(callback_).Run(std::move(message));
  return true;
}

gin::WrapperInfo ReplyChannel::kWrapperInfo = {gin::kEmbedderNativeGin};

}  // namespace gin_helper::internal
