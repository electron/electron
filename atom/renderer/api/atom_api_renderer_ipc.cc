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
#include "electron/atom/common/api/api.mojom.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
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

class IPCRenderer : public mate::Wrappable<IPCRenderer> {
 public:
  explicit IPCRenderer(v8::Isolate* isolate) {
    Init(isolate);
    RenderFrame* render_frame = GetCurrentRenderFrame();
    DCHECK(render_frame);
    render_frame->GetRemoteAssociatedInterfaces()->GetInterface(
        mojo::MakeRequest(&electron_browser_ptr_));
  }
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
    prototype->SetClassName(mate::StringToV8(isolate, "IPCRenderer"));
    mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
        .SetMethod("send", &IPCRenderer::Send)
        .SetMethod("sendSync", &IPCRenderer::SendSync);
  }
  static mate::Handle<IPCRenderer> Create(v8::Isolate* isolate) {
    return mate::CreateHandle(isolate, new IPCRenderer(isolate));
  }

  void Send(mate::Arguments* args,
            bool internal,
            const std::string& channel,
            const base::ListValue& arguments) {
    electron_browser_ptr_->Message(internal, channel, arguments.Clone());
  }

  base::Value SendSync(mate::Arguments* args,
                       bool internal,
                       const std::string& channel,
                       const base::ListValue& arguments) {
    base::Value result;
    electron_browser_ptr_->MessageSync(internal, channel, arguments.Clone(),
                                       &result);
    return result;
  }

 private:
  atom::mojom::ElectronBrowserAssociatedPtr electron_browser_ptr_;
};

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
  dict.Set("ipc", IPCRenderer::Create(context->GetIsolate()));
  dict.SetMethod("sendTo", &SendTo);
  dict.SetMethod("sendToHost", &SendToHost);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_renderer_ipc, Initialize)
