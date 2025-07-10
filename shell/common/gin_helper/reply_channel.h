// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_

#include <string_view>

#include "gin/wrappable.h"
#include "shell/common/api/api.mojom.h"
#include "v8/include/cppgc/prefinalizer.h"

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace v8 {
class Isolate;
template <typename T>
class Local;
class Object;
class ObjectTemplate;
}  // namespace v8

namespace gin_helper::internal {

// This object wraps the InvokeCallback so that if it gets GC'd by V8, we can
// still call the callback and send an error. Not doing so causes a Mojo DCHECK,
// since Mojo requires callbacks to be called before they are destroyed.
class ReplyChannel : public gin::Wrappable<ReplyChannel> {
  CPPGC_USING_PRE_FINALIZER(ReplyChannel, EnsureReplySent);

 public:
  using InvokeCallback = electron::mojom::ElectronApiIPC::InvokeCallback;

  static ReplyChannel* Create(v8::Isolate* isolate, InvokeCallback callback);

  // Constructor is public only to satisfy cppgc::MakeGarbageCollected.
  // Callers should use Create() instead.
  explicit ReplyChannel(v8::Isolate* isolate, InvokeCallback callback);
  ~ReplyChannel() override;

  // disable copy
  ReplyChannel(const ReplyChannel&) = delete;
  ReplyChannel& operator=(const ReplyChannel&) = delete;

  // gin_helper::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // Invokes `callback` (if it's non-null) with `errmsg` as an arg.
  static void SendError(v8::Isolate* isolate,
                        InvokeCallback callback,
                        std::string_view errmsg);

  void EnsureReplySent();

 private:
  static bool SendReplyImpl(v8::Isolate* isolate,
                            InvokeCallback callback,
                            v8::Local<v8::Value> arg);

  bool SendReply(v8::Isolate* isolate, v8::Local<v8::Value> arg);

  InvokeCallback callback_;
};

}  // namespace gin_helper::internal

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_
