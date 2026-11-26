// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/reply_channel.h"

#include "base/task/single_thread_task_runner.h"
#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "shell/common/gin_converters/serialized_value_converter.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-isolate.h"

namespace gin_helper::internal {

const gin::WrapperInfo ReplyChannel::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronReplyChannel);

ReplyChannel::ReplyChannel(v8::Isolate* isolate, InvokeCallback callback)
    : callback_{std::move(callback)} {}

ReplyChannel::~ReplyChannel() {
  if (!callback_)
    return;
  // Post a task because we cannot allocate on the V8 heap during cppgc
  // sweeping, which ReplyChannel::SendError requires.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](InvokeCallback callback) {
            if (v8::Isolate* isolate = v8::Isolate::TryGetCurrent()) {
              ReplyChannel::SendError(isolate, std::move(callback),
                                      "reply was never sent");
            }
          },
          std::move(callback_)));
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

  SendReplyImpl(isolate, callback,
                gin::DataObjectBuilder(isolate).Set("error", errmsg).Build());
}

// static
bool ReplyChannel::SendReplyImpl(v8::Isolate* isolate,
                                 InvokeCallback& callback,
                                 v8::Local<v8::Value> arg) {
  if (!callback)
    return false;

  // Serialize first so a failed conversion leaves |callback| usable.
  electron::SerializedValue msg;
  if (!gin::ConvertFromV8(isolate, arg, &msg))
    return false;
  // Serialization can run JS that replied re-entrantly.
  if (!callback)
    return false;

  std::move(callback).Run(std::move(msg));
  return true;
}

// static
ReplyChannel* ReplyChannel::Create(v8::Isolate* isolate,
                                   InvokeCallback callback) {
  return cppgc::MakeGarbageCollected<ReplyChannel>(
      isolate->GetCppHeap()->GetAllocationHandle(), isolate,
      std::move(callback));
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

bool ReplyChannel::SendReply(v8::Isolate* isolate, v8::Local<v8::Value> arg) {
  return SendReplyImpl(isolate, callback_, arg);
}

}  // namespace gin_helper::internal
