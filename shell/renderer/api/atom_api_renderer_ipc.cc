// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "base/task/post_task.h"
#include "base/values.h"
#include "content/public/renderer/render_frame.h"
#include "gin/dictionary.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"
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

class IPCRenderer : public gin::Wrappable<IPCRenderer> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static gin::Handle<IPCRenderer> Create(v8::Isolate* isolate) {
    return gin::CreateHandle(isolate, new IPCRenderer(isolate));
  }

  explicit IPCRenderer(v8::Isolate* isolate)
      : task_runner_(base::CreateSingleThreadTaskRunner({base::ThreadPool()})) {
    RenderFrame* render_frame = GetCurrentRenderFrame();
    DCHECK(render_frame);

    // Bind the interface on the background runner. All accesses will be via
    // the thread-safe pointer. This is to support our "fake-sync"
    // MessageSync() hack; see the comment in IPCRenderer::SendSync.
    electron::mojom::ElectronBrowserPtrInfo info;
    render_frame->GetRemoteInterfaces()->GetInterface(mojo::MakeRequest(&info));
    electron_browser_ptr_ =
        electron::mojom::ThreadSafeElectronBrowserPtr::Create(std::move(info),
                                                              task_runner_);
  }

  // gin::Wrappable:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<IPCRenderer>::GetObjectTemplateBuilder(isolate)
        .SetMethod("send", &IPCRenderer::Send)
        .SetMethod("sendSync", &IPCRenderer::SendSync)
        .SetMethod("sendTo", &IPCRenderer::SendTo)
        .SetMethod("sendToHost", &IPCRenderer::SendToHost)
        .SetMethod("invoke", &IPCRenderer::Invoke);
  }

  const char* GetTypeName() override { return "IPCRenderer"; }

 private:
  void Send(v8::Isolate* isolate,
            bool internal,
            const std::string& channel,
            v8::Local<v8::Value> arguments) {
    blink::CloneableMessage message;
    if (!gin::ConvertFromV8(isolate, arguments, &message)) {
      return;
    }
    electron_browser_ptr_->get()->Message(internal, channel,
                                          std::move(message));
  }

  v8::Local<v8::Promise> Invoke(v8::Isolate* isolate,
                                bool internal,
                                const std::string& channel,
                                v8::Local<v8::Value> arguments) {
    blink::CloneableMessage message;
    if (!gin::ConvertFromV8(isolate, arguments, &message)) {
      return v8::Local<v8::Promise>();
    }
    electron::util::Promise<blink::CloneableMessage> p(isolate);
    auto handle = p.GetHandle();

    electron_browser_ptr_->get()->Invoke(
        internal, channel, std::move(message),
        base::BindOnce(
            [](electron::util::Promise<blink::CloneableMessage> p,
               blink::CloneableMessage result) { p.ResolveWithGin(result); },
            std::move(p)));

    return handle;
  }

  void SendTo(v8::Isolate* isolate,
              bool internal,
              bool send_to_all,
              int32_t web_contents_id,
              const std::string& channel,
              v8::Local<v8::Value> arguments) {
    blink::CloneableMessage message;
    if (!gin::ConvertFromV8(isolate, arguments, &message)) {
      return;
    }
    electron_browser_ptr_->get()->MessageTo(
        internal, send_to_all, web_contents_id, channel, std::move(message));
  }

  void SendToHost(v8::Isolate* isolate,
                  const std::string& channel,
                  v8::Local<v8::Value> arguments) {
    blink::CloneableMessage message;
    if (!gin::ConvertFromV8(isolate, arguments, &message)) {
      return;
    }
    electron_browser_ptr_->get()->MessageHost(channel, std::move(message));
  }

  blink::CloneableMessage SendSync(v8::Isolate* isolate,
                                   bool internal,
                                   const std::string& channel,
                                   v8::Local<v8::Value> arguments) {
    blink::CloneableMessage message;
    if (!gin::ConvertFromV8(isolate, arguments, &message)) {
      return blink::CloneableMessage();
    }
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
    // of a trick: we pass the Mojo handle over to a worker thread, send the
    // asynchronous message from that thread, and then block on the result.
    // It's important that we pass the handle over to the worker thread,
    // because that allows Mojo to process incoming messages (most importantly,
    // the response to our request) on that thread. If we didn't pass it to a
    // worker thread, and instead sent the call from the main thread, we would
    // never receive a response because Mojo wouldn't be able to run its
    // message handling code, because the main thread would be tied up blocking
    // on the WaitableEvent.
    //
    // Phew. If you got this far, here's a gold star: ⭐️

    blink::CloneableMessage result;

    // A task is posted to a worker thread to execute the request so that
    // this thread may block on a waitable event. It is safe to pass raw
    // pointers to |result| and |response_received_event| as this stack frame
    // will survive until the request is complete.

    base::WaitableEvent response_received_event;
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&IPCRenderer::SendMessageSyncOnWorkerThread,
                                  base::Unretained(this),
                                  base::Unretained(&response_received_event),
                                  base::Unretained(&result), internal, channel,
                                  std::move(message)));
    response_received_event.Wait();
    return result;
  }

  void SendMessageSyncOnWorkerThread(base::WaitableEvent* event,
                                     blink::CloneableMessage* result,
                                     bool internal,
                                     const std::string& channel,
                                     blink::CloneableMessage arguments) {
    electron_browser_ptr_->get()->MessageSync(
        internal, channel, std::move(arguments),
        base::BindOnce(&IPCRenderer::ReturnSyncResponseToMainThread,
                       base::Unretained(event), base::Unretained(result)));
  }

  static void ReturnSyncResponseToMainThread(base::WaitableEvent* event,
                                             blink::CloneableMessage* result,
                                             blink::CloneableMessage response) {
    *result = std::move(response);
    event->Signal();
  }

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  scoped_refptr<electron::mojom::ThreadSafeElectronBrowserPtr>
      electron_browser_ptr_;
};

gin::WrapperInfo IPCRenderer::kWrapperInfo = {gin::kEmbedderNativeGin};

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin::Dictionary dict(context->GetIsolate(), exports);
  dict.Set("ipc", IPCRenderer::Create(context->GetIsolate()));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_renderer_ipc, Initialize)
