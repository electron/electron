// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_renderer_ipc.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "content/public/renderer/render_frame.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

using blink::WebLocalFrame;
using content::RenderFrame;

namespace atom {

namespace api {

RenderFrame* GetCurrentRenderFrame() {
  WebLocalFrame* frame = WebLocalFrame::FrameForCurrentContext();
  if (!frame)
    return nullptr;

  return RenderFrame::FromWebFrame(frame);
}

void Send(mate::Arguments* args,
          const base::string16& channel,
          const base::ListValue& arguments) {
  RenderFrame* render_frame = GetCurrentRenderFrame();
  if (render_frame == nullptr)
    return;

  bool success = render_frame->Send(new AtomFrameHostMsg_Message(
      render_frame->GetRoutingID(), channel, arguments));

  if (!success)
    args->ThrowError("Unable to send AtomFrameHostMsg_Message");
}

base::ListValue SendSync(mate::Arguments* args,
                         const base::string16& channel,
                         const base::ListValue& arguments) {
  base::ListValue result;

  RenderFrame* render_frame = GetCurrentRenderFrame();
  if (render_frame == nullptr)
    return result;

  IPC::SyncMessage* message = new AtomFrameHostMsg_Message_Sync(
      render_frame->GetRoutingID(), channel, arguments, &result);
  bool success = render_frame->Send(message);

  if (!success)
    args->ThrowError("Unable to send AtomFrameHostMsg_Message_Sync");

  return result;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("send", &Send);
  dict.SetMethod("sendSync", &SendSync);
}

}  // namespace api

}  // namespace atom

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_renderer_ipc, atom::api::Initialize)
