// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/reply_channel.h"

#include <string>
#include <utility>

#include "gin/object_template_builder.h"
#include "shell/common/gin_converters/serialized_value_converter.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-isolate.h"

namespace gin_helper::internal {

const gin::WrapperInfo ReplyChannel::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronReplyChannel);

ReplyChannel::ReplyChannel(Callback callback)
    : callback_{std::move(callback)} {}

ReplyChannel::~ReplyChannel() {
  SendError("reply was never sent");
}

// static
void ReplyChannel::SendError(Callback callback, std::string_view errmsg) {
  if (auto* invoke = std::get_if<InvokeCallback>(&callback)) {
    if (*invoke) {
      std::move(*invoke).Run(
          electron::mojom::InvokeResult::NewError(std::string(errmsg)));
    }
  } else if (auto* sync = std::get_if<MessageSyncCallback>(&callback)) {
    if (*sync) {
      std::move(*sync).Run(electron::SerializedValue());
    }
  }
}

void ReplyChannel::SendError(std::string_view errmsg) {
  SendError(std::move(callback_), errmsg);
  callback_.emplace<std::monostate>();
}

// static
ReplyChannel* ReplyChannel::Create(v8::Isolate* isolate, Callback callback) {
  return cppgc::MakeGarbageCollected<ReplyChannel>(
      isolate->GetCppHeap()->GetAllocationHandle(), std::move(callback));
}

const gin::WrapperInfo* ReplyChannel::wrapper_info() const {
  return &kWrapperInfo;
}

const char* ReplyChannel::GetHumanReadableName() const {
  return "Electron / ReplyChannel";
}

gin::ObjectTemplateBuilder ReplyChannel::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<ReplyChannel>::GetObjectTemplateBuilder(isolate)
      .SetMethod("sendReply", &ReplyChannel::SendReply);
}

bool ReplyChannel::HasCallback() const {
  if (const auto* invoke = std::get_if<InvokeCallback>(&callback_))
    return static_cast<bool>(*invoke);
  if (const auto* sync = std::get_if<MessageSyncCallback>(&callback_))
    return static_cast<bool>(*sync);
  return false;
}

bool ReplyChannel::SendReply(v8::Isolate* isolate, v8::Local<v8::Value> arg) {
  if (!HasCallback())
    return false;

  // Serialize first so a failed conversion leaves |callback_| usable.
  electron::SerializedValue msg;
  if (!gin::ConvertFromV8(isolate, arg, &msg))
    return false;
  // Serialization can run JS that replied re-entrantly.
  if (!HasCallback())
    return false;

  if (auto* invoke = std::get_if<InvokeCallback>(&callback_)) {
    std::move(*invoke).Run(
        electron::mojom::InvokeResult::NewResult(std::move(msg)));
  } else if (auto* sync = std::get_if<MessageSyncCallback>(&callback_)) {
    std::move(*sync).Run(std::move(msg));
  }
  callback_.emplace<std::monostate>();
  return true;
}

}  // namespace gin_helper::internal
