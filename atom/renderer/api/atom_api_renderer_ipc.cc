// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "third_party/blink/public/web/web_local_frame.h"

using blink::WebLocalFrame;
using content::RenderFrame;

namespace {

RenderFrame* GetCurrentRenderFrame() {
  WebLocalFrame* frame = WebLocalFrame::FrameForCurrentContext();
  if (!frame)
    return nullptr;

  return RenderFrame::FromWebFrame(frame);
}

void Send(mate::Arguments* args,
          bool internal,
          const std::string& channel,
          const base::ListValue& arguments) {
  RenderFrame* render_frame = GetCurrentRenderFrame();
  if (render_frame == nullptr)
    return;

  bool success = render_frame->Send(new AtomFrameHostMsg_Message(
      render_frame->GetRoutingID(), internal, channel, arguments));

  if (!success)
    args->ThrowError("Unable to send AtomFrameHostMsg_Message");
}

base::ListValue SendSync(mate::Arguments* args,
                         bool internal,
                         const std::string& channel,
                         const base::ListValue& arguments) {
  base::ListValue result;

  RenderFrame* render_frame = GetCurrentRenderFrame();
  if (render_frame == nullptr)
    return result;

  IPC::SyncMessage* message = new AtomFrameHostMsg_Message_Sync(
      render_frame->GetRoutingID(), internal, channel, arguments, &result);
  bool success = render_frame->Send(message);

  if (!success)
    args->ThrowError("Unable to send AtomFrameHostMsg_Message_Sync");

  return result;
}

void SendTo(mate::Arguments* args,
            bool internal,
            bool send_to_all,
            int32_t web_contents_id,
            const std::string& channel,
            const base::ListValue& arguments) {
  RenderFrame* render_frame = GetCurrentRenderFrame();
  if (render_frame == nullptr)
    return;

  bool success = render_frame->Send(new AtomFrameHostMsg_Message_To(
      render_frame->GetRoutingID(), internal, send_to_all, web_contents_id,
      channel, arguments));

  if (!success)
    args->ThrowError("Unable to send AtomFrameHostMsg_Message_To");
}

void SendToHost(mate::Arguments* args,
                const std::string& channel,
                const base::ListValue& arguments) {
  RenderFrame* render_frame = GetCurrentRenderFrame();
  if (render_frame == nullptr)
    return;

  bool success = render_frame->Send(new AtomFrameHostMsg_Message_Host(
      render_frame->GetRoutingID(), channel, arguments));

  if (!success)
    args->ThrowError("Unable to send AtomFrameHostMsg_Message_Host");
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("send", &Send);
  dict.SetMethod("sendSync", &SendSync);
  dict.SetMethod("sendTo", &SendTo);
  dict.SetMethod("sendToHost", &SendToHost);
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_renderer_ipc, Initialize)
