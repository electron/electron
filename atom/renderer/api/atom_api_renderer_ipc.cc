// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "base/task/post_task.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "electron/atom/common/api/api.mojom.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable.h"
#include "services/service_manager/public/cpp/interface_provider.h"
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
        .SetMethod("sendToHost", &IPCRenderer::SendToHost);
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

  void SendTo(mate::Arguments* args,
              bool internal,
              bool send_to_all,
              int32_t web_contents_id,
              const std::string& channel,
              const base::ListValue& arguments) {
    electron_browser_ptr_->MessageTo(internal, send_to_all, web_contents_id,
                                     channel, arguments.Clone());
  }

  void SendToHost(mate::Arguments* args,
                  const std::string& channel,
                  const base::ListValue& arguments) {
    electron_browser_ptr_->MessageHost(channel, arguments.Clone());
  }

  base::Value SendSync(mate::Arguments* args,
                       bool internal,
                       const std::string& channel,
                       const base::ListValue& arguments) {
    base::Value result;

    // A task is posted to a separate thread to execute the request so that
    // this thread may block on a waitable event. It is safe to pass raw
    // pointers to |result| and |event| as this stack frame will survive until
    // the request is complete.
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        base::CreateSingleThreadTaskRunnerWithTraits({});

    base::WaitableEvent response_received_event;

    // We unbind the interface from this thread to pass it over to the worker
    // thread temporarily. This requires that no callbacks be pending for this
    // interface.
    auto interface_info = electron_browser_ptr_.PassInterface();
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(&IPCRenderer::SendMessageSyncOnWorkerThread,
                                  base::Unretained(&interface_info),
                                  base::Unretained(&response_received_event),
                                  base::Unretained(&result), internal, channel,
                                  base::Unretained(&arguments)));
    response_received_event.Wait();
    electron_browser_ptr_.Bind(std::move(interface_info));
    return result;
  }

 private:
  static void SendMessageSyncOnWorkerThread(
      atom::mojom::ElectronBrowserPtrInfo* interface_info,
      base::WaitableEvent* event,
      base::Value* result,
      bool internal,
      const std::string& channel,
      const base::ListValue* arguments) {
    atom::mojom::ElectronBrowserPtr browser_ptr(std::move(*interface_info));
    browser_ptr->MessageSync(
        internal, channel, arguments->Clone(),
        base::BindOnce(&IPCRenderer::ReturnSyncResponseToMainThread,
                       std::move(browser_ptr), base::Unretained(interface_info),
                       base::Unretained(event), base::Unretained(result)));
  }
  static void ReturnSyncResponseToMainThread(
      atom::mojom::ElectronBrowserPtr ptr,
      atom::mojom::ElectronBrowserPtrInfo* interface_info,
      base::WaitableEvent* event,
      base::Value* result,
      base::Value response) {
    *result = std::move(response);
    *interface_info = ptr.PassInterface();
    event->Signal();
  }

  atom::mojom::ElectronBrowserPtr electron_browser_ptr_;
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
