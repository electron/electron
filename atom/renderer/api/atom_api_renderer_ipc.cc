// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "atom/common/promise_util.h"
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
    render_frame->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&electron_browser_async_ptr_));
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

  void Send(mate::Arguments* args,
            bool internal,
            const std::string& channel,
            const base::ListValue& arguments) {
    electron_browser_ptr_->Message(internal, channel, arguments.Clone());
  }

  v8::Local<v8::Promise> Invoke(mate::Arguments* args,
                                const std::string& channel,
                                const base::Value& arguments) {
    atom::util::Promise p(args->isolate());
    auto handle = p.GetHandle();
    electron_browser_async_ptr_->Invoke(
        channel, arguments.Clone(),
        base::BindOnce(
            [](atom::util::Promise p, base::Value value) { p.Resolve(value); },
            std::move(p)));

    return handle;
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
    // We aren't using a true synchronous mojo call here. We're calling an
    // asynchronous method and blocking on the result. The reason we're doing
    // this is a little complicated, so buckle up.
    //
    // Mojo has a concept of synchronous calls. However, synchronous calls are
    // dangerous. In particular, it's quite possible for two processes to call
    // synchronous methods on each other and cause a deadlock. Mojo has a
    // mechanism to avoid this kind of deadlock: if a process is waiting on the
    // result of a synchronous call, and it receives an incoming call for a
    // synchronous method, it will process that request immediately, even
    // though it's currently blocking. However, if it receives an incoming
    // request for an _asynchronous_ method, that can't cause a deadlock, so it
    // stashes the request on a queue to be processed once the synchronous
    // thing it's waiting on returns.
    //
    // This behavior is useful for preventing deadlocks, but it is inconvenient
    // here because it can result in messages being reordered. If the main
    // process is awaiting the result of a synchronous call (which it does only
    // very rarely, since it's bad to block the main process), and we send
    // first an asynchronous message to the main process, followed by a
    // synchronous message, then the main process will process the synchronous
    // one first.
    //
    // It turns out, Electron has some dependency on message ordering,
    // especially during window shutdown, and getting messages out of order can
    // result in, for example, remote objects disappearing unexpectedly. To
    // avoid these issues and guarantee consistent message ordering, we send
    // all messages to the main process as asynchronous messages. This causes
    // them to always be queued and processed in the same order they were
    // received, even if they were received while the main process was waiting
    // on a synchronous call.
    //
    // However, in the calling process, we still need to block on the result,
    // because the caller is expecting a result synchronously. So we do a bit
    // of a trick: we pass the Mojo handle over to a new thread, send the
    // asynchronous message from that thread, and then block on the result.
    // It's important that we pass the handle over to the new thread, because
    // that allows Mojo to process incoming messages (most importantly, the
    // response to our request) on the new thread. If we didn't pass it to a
    // new thread, and instead sent the call from the main thread, we would
    // never receive a response because Mojo wouldn't be able to run its
    // message handling code, because the main thread would be tied up blocking
    // on the WaitableEvent.
    //
    // Phew. If you got this far, here's a gold star: ⭐️

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

  atom::mojom::ElectronBrowserPtr electron_browser_async_ptr_;
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
