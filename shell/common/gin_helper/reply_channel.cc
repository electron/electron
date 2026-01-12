// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/reply_channel.h"

#include "base/debug/stack_trace.h"
#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_helper/handle.h"

namespace gin_helper::internal {

// static
using InvokeCallback = electron::mojom::ElectronApiIPC::InvokeCallback;
gin_helper::Handle<ReplyChannel> ReplyChannel::Create(v8::Isolate* isolate,
                                                      InvokeCallback callback) {
  return gin_helper::CreateHandle(isolate,
                                  new ReplyChannel(std::move(callback)));
}

gin::ObjectTemplateBuilder ReplyChannel::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::DeprecatedWrappable<
             ReplyChannel>::GetObjectTemplateBuilder(isolate)
      .SetMethod("sendReply", &ReplyChannel::SendReply);
}

const char* ReplyChannel::GetTypeName() {
  return "ReplyChannel";
}

ReplyChannel::ReplyChannel(InvokeCallback callback)
    : callback_(std::move(callback)) {}

ReplyChannel::~ReplyChannel() {
  if (callback_)
    SendError(electron::JavascriptEnvironment::GetIsolate(),
              std::move(callback_), "reply was never sent");
}

// static
void ReplyChannel::SendError(v8::Isolate* isolate,
                             InvokeCallback callback,
                             std::string_view const errmsg) {
  if (!callback)
    return;

  // If there's no current context, it means we're shutting down,
  // so we don't need to send an event.
  v8::HandleScope scope{isolate};
  if (isolate->GetCurrentContext().IsEmpty())
    return;

  SendReplyImpl(isolate, std::move(callback),
                gin::DataObjectBuilder(isolate).Set("error", errmsg).Build());
}

// static
bool ReplyChannel::SendReplyImpl(v8::Isolate* isolate,
                                 InvokeCallback callback,
                                 v8::Local<v8::Value> arg) {
  if (!callback)
    return false;

  blink::CloneableMessage msg;
  if (!gin::ConvertFromV8(isolate, arg, &msg))
    return false;

  std::move(callback).Run(std::move(msg));
  return true;
}

bool ReplyChannel::SendReply(v8::Isolate* isolate, v8::Local<v8::Value> arg) {
  return SendReplyImpl(isolate, std::move(callback_), std::move(arg));
}

gin::DeprecatedWrapperInfo ReplyChannel::kWrapperInfo = {
    gin::kEmbedderNativeGin};

}  // namespace gin_helper::internal
