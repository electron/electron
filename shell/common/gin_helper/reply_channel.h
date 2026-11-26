// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_

#include <string_view>
#include <variant>

#include "gin/wrappable.h"
#include "shell/common/api/api.mojom.h"

namespace v8 {
class Isolate;
template <typename T>
class Local;
}  // namespace v8

namespace gin_helper::internal {

// This object wraps a Mojo reply callback (Invoke or MessageSync) so that if
// it gets GC'd by V8, we can still run the callback and send an error or empty
// reply. Not doing so causes a Mojo DCHECK, since Mojo requires reply
// callbacks to be called before they are destroyed.
class ReplyChannel : public gin::Wrappable<ReplyChannel> {
 public:
  using InvokeCallback = electron::mojom::ElectronApiIPC::InvokeCallback;
  using MessageSyncCallback =
      electron::mojom::ElectronApiIPC::MessageSyncCallback;
  using Callback =
      std::variant<std::monostate, InvokeCallback, MessageSyncCallback>;

  static ReplyChannel* Create(v8::Isolate* isolate, Callback callback);

  // Constructor is public only to satisfy cppgc::MakeGarbageCollected.
  // Callers should use Create() instead.
  explicit ReplyChannel(Callback callback);
  ~ReplyChannel() override;

  // disable copy
  ReplyChannel(const ReplyChannel&) = delete;
  ReplyChannel& operator=(const ReplyChannel&) = delete;

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // Invokes |callback| (if it's non-null) with |errmsg| for Invoke, or an
  // empty reply for MessageSync.
  static void SendError(Callback callback, std::string_view errmsg);

  // Sends an error reply on this channel if it has not been replied to yet.
  void SendError(std::string_view errmsg);

  // False if |arg| cannot be serialized (the channel stays usable) or a
  // reply was already sent.
  bool SendReply(v8::Isolate* isolate, v8::Local<v8::Value> arg);

 private:
  bool HasCallback() const;

  Callback callback_;
};

}  // namespace gin_helper::internal

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_REPLY_CHANNEL_H_
