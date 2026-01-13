// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/reply_channel.h"

#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_helper/handle.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

namespace gin_helper::internal {

// static
using InvokeCallback = electron::mojom::ElectronApiIPC::InvokeCallback;
ReplyChannel* ReplyChannel::Create(v8::Isolate* isolate,
                                   InvokeCallback callback) {
  return cppgc::MakeGarbageCollected<ReplyChannel>(
      isolate->GetCppHeap()->GetAllocationHandle(), isolate,
      std::move(callback));
}

ReplyChannel::ReplyChannel(v8::Isolate* isolate, InvokeCallback callback)
    : isolate_{isolate},
      callback_{std::move(callback)},
      per_isolate_data_{gin::PerIsolateData::From(isolate)} {
  per_isolate_data_->AddDisposeObserver(this);
}

ReplyChannel::~ReplyChannel() {
  EnsureReplySent(isolate_);
}

// Wrappable

const gin::WrapperInfo ReplyChannel::kWrapperInfo = {
    {gin::kEmbedderNativeGin},
    gin::kElectronReplyChannel};

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

// gin::PerIsolateData::DisposeObserver

void ReplyChannel::OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) {
  EnsureReplySent(isolate);
}

void ReplyChannel::OnDisposed() {
  per_isolate_data_ = nullptr;
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

void ReplyChannel::EnsureReplySent(v8::Isolate* const isolate) {
  if (callback_) {
    DCHECK(isolate);
    SendError(isolate, std::move(callback_), "reply was never sent");
  }

  if (per_isolate_data_) {
    per_isolate_data_->RemoveDisposeObserver(this);
    per_isolate_data_ = nullptr;
  }
}

}  // namespace gin_helper::internal
