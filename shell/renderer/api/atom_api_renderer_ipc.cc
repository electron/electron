// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "base/task/post_task.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "electron/shell/common/api/api.mojom.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/common/native_mate_converters/value_converter.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/promise_util.h"
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

    render_frame->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&electron_browser_ptr_));
  }

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
    prototype->SetClassName(mate::StringToV8(isolate, "IPCRenderer"));
    mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
        .SetMethod("send", &IPCRenderer::Send)
        .SetMethod("sendSync", &IPCRenderer::SendSync)
        .SetMethod("sendTo", &IPCRenderer::SendTo)
        .SetMethod("sendToHost", &IPCRenderer::SendToHost)
        .SetMethod("invoke", &IPCRenderer::Invoke);
  }

  static mate::Handle<IPCRenderer> Create(v8::Isolate* isolate) {
    return mate::CreateHandle(isolate, new IPCRenderer(isolate));
  }

  void Send(bool internal,
            const std::string& channel,
            const base::ListValue& arguments) {
    electron_browser_ptr_->Message(
        internal, channel, base::ListValue(arguments.Clone().GetList()));
  }

  v8::Local<v8::Promise> Invoke(mate::Arguments* args,
                                const std::string& channel,
                                const base::Value& arguments) {
    electron::util::Promise p(args->isolate());
    auto handle = p.GetHandle();

    electron_browser_ptr_->Invoke(
        channel, base::ListValue(arguments.Clone().GetList()),
        base::BindOnce([](electron::util::Promise p,
                          base::Value result) { p.Resolve(result); },
                       std::move(p)));

    return handle;
  }

  void SendTo(bool internal,
              bool send_to_all,
              int32_t web_contents_id,
              const std::string& channel,
              const base::ListValue& arguments) {
    electron_browser_ptr_->MessageTo(
        internal, send_to_all, web_contents_id, channel,
        base::ListValue(arguments.Clone().GetList()));
  }

  void SendToHost(const std::string& channel,
                  const base::ListValue& arguments) {
    electron_browser_ptr_->MessageHost(
        channel, base::ListValue(arguments.Clone().GetList()));
  }

  base::Value SendSync(bool internal,
                       const std::string& channel,
                       const base::ListValue& arguments) {
    base::Value result;

    electron_browser_ptr_->MessageSync(
        internal, channel, base::ListValue(arguments.Clone().GetList()),
        &result);
    return result;
  }

 private:
  electron::mojom::ElectronBrowserPtr electron_browser_ptr_;
};

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.Set("ipc", IPCRenderer::Create(context->GetIsolate()));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_renderer_ipc, Initialize)
