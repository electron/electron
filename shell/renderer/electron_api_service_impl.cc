// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/electron_api_service_impl.h"

#include <utility>
#include <vector>

#include "gin/converter.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/serialized_value_converter.h"
#include "shell/common/heap_snapshot.h"
#include "shell/common/thread_restrictions.h"
#include "shell/common/v8_util.h"
#include "shell/renderer/electron_ipc_native.h"

#include "base/functional/callback_helpers.h"
#include "base/no_destructor.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "shell/renderer/renderer_client_base.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_input_method_controller.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_message_port_converter.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/base/ime/ime_text_span.h"
#include "url/gurl.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-external.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-promise.h"

namespace electron {

namespace {

mojom::RendererStartupDataPtr* GetPendingNewWindowStartupData() {
  // Process-global, no locking: set, RenderFrame creation, and take are all
  // one synchronous call stack inside window.open() on the renderer main
  // thread.
  DCHECK(content::RenderThread::Get())
      << "must be called from the renderer main thread";
  static base::NoDestructor<mojom::RendererStartupDataPtr> pending;
  return pending.get();
}

}  // namespace

namespace {

// Replies to an ExecuteJavaScript() call with the script's completion value,
// awaiting it first if it is a thenable, or with the error. Owns itself.
class ScriptReply {
 public:
  using Callback = mojom::ElectronFrame::ExecuteJavaScriptCallback;

  explicit ScriptReply(Callback callback) : callback_(std::move(callback)) {}
  ScriptReply(const ScriptReply&) = delete;
  ScriptReply& operator=(const ScriptReply&) = delete;

  void Completed(const std::vector<v8::Local<v8::Value>>& result) {
    if (result.empty()) {
      return Fail(
          "WebFrame was removed before script could run. This normally means "
          "the underlying frame was destroyed");
    }
    v8::Local<v8::Value> value = result[0];
    if (value.IsEmpty()) {
      return Fail(
          "Script failed to execute, this normally means an error was thrown. "
          "Check the renderer console for the error.");
    }
    if (!value->IsPromise())
      return Settle(true, value);

    // Wait for the promise; the two handlers share ownership of |this| through
    // the External and whichever runs first deletes it.
    v8::Local<v8::Promise> promise = value.As<v8::Promise>();
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> context =
        promise->GetCreationContextChecked(isolate);
    v8::Context::Scope context_scope(context);
    v8::Local<v8::External> self =
        v8::External::New(isolate, this, v8::kExternalPointerTypeTagDefault);
    v8::Local<v8::Function> on_fulfilled, on_rejected;
    if (!v8::Function::New(context, &ScriptReply::OnFulfilled, self, 1,
                           v8::ConstructorBehavior::kThrow)
             .ToLocal(&on_fulfilled) ||
        !v8::Function::New(context, &ScriptReply::OnRejected, self, 1,
                           v8::ConstructorBehavior::kThrow)
             .ToLocal(&on_rejected) ||
        promise->Then(context, on_fulfilled, on_rejected).IsEmpty()) {
      return Fail("Failed to await the script's result");
    }
  }

 private:
  ~ScriptReply() = default;

  static void OnFulfilled(const v8::FunctionCallbackInfo<v8::Value>& info) {
    Take(info)->Settle(true, info[0]);
  }
  static void OnRejected(const v8::FunctionCallbackInfo<v8::Value>& info) {
    Take(info)->Settle(false, info[0]);
  }
  static ScriptReply* Take(const v8::FunctionCallbackInfo<v8::Value>& info) {
    return static_cast<ScriptReply*>(info.Data().As<v8::External>()->Value(
        v8::kExternalPointerTypeTagDefault));
  }

  void Fail(std::string_view message) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    Settle(false, v8::Exception::Error(gin::StringToV8(isolate, message)));
  }

  void Settle(bool success, v8::Local<v8::Value> value) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    electron::SerializedValue serialized;
    bool ok;
    {
      v8::TryCatch try_catch(isolate);
      ok = electron::SerializeV8Value(isolate, value, &serialized);
      if (!ok) {
        success = false;
        v8::Local<v8::Value> error =
            try_catch.HasCaught() && !try_catch.HasTerminated()
                ? try_catch.Exception()
                : v8::Exception::Error(gin::StringToV8(
                      isolate, "An object could not be cloned."));
        try_catch.Reset();
        ok = electron::SerializeV8Value(isolate, error, &serialized);
      }
    }
    if (!ok) {
      serialized = electron::SerializedValue();
      success = false;
    }
    std::move(callback_).Run(success, std::move(serialized));
    delete this;
  }

  Callback callback_;
};

}  // namespace

ElectronApiServiceImpl::~ElectronApiServiceImpl() = default;

ElectronApiServiceImpl::ElectronApiServiceImpl(
    content::RenderFrame* render_frame,
    RendererClientBase* renderer_client)
    : content::RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<ElectronApiServiceImpl>(render_frame),
      renderer_client_(renderer_client) {
  registry_.AddInterface<mojom::ElectronRenderer>(base::BindRepeating(
      &ElectronApiServiceImpl::BindTo, base::Unretained(this)));
  // Associated with content.mojom.Frame, so SetStartupData() arrives before
  // the CommitNavigation that follows it — i.e. before DidCreateScriptContext.
  render_frame->GetAssociatedInterfaceRegistry()
      ->AddInterface<mojom::ElectronFrame>(base::BindRepeating(
          &ElectronApiServiceImpl::BindFrameReceiver, base::Unretained(this)));

  // window.open() popup's about:blank fires DidCreateScriptContext before
  // any push can land; the browser attaches its startup data to the
  // CreateNewWindowReply and SetPendingCreateNewWindowStartupData() stashes it
  // for us on this same call stack. The first real navigation replaces it.
  startup_data_ = std::exchange(*GetPendingNewWindowStartupData(), nullptr);
}

// static
void ElectronApiServiceImpl::SetPendingNewWindowStartupData(
    mojom::RendererStartupDataPtr data) {
  *GetPendingNewWindowStartupData() = std::move(data);
}

void ElectronApiServiceImpl::BindFrameReceiver(
    mojo::PendingAssociatedReceiver<mojom::ElectronFrame> receiver) {
  if (frame_receiver_.is_bound())
    frame_receiver_.reset();
  frame_receiver_.Bind(std::move(receiver));
}

void ElectronApiServiceImpl::SetStartupData(
    mojom::RendererStartupDataPtr data) {
  startup_data_ = std::move(data);
}

void ElectronApiServiceImpl::BindTo(
    mojo::PendingReceiver<mojom::ElectronRenderer> receiver) {
  if (document_created_) {
    if (receiver_.is_bound())
      receiver_.reset();

    receiver_.Bind(std::move(receiver));
    receiver_.set_disconnect_handler(base::BindOnce(
        &ElectronApiServiceImpl::OnConnectionError, GetWeakPtr()));
  } else {
    pending_receiver_ = std::move(receiver);
  }
}

void ElectronApiServiceImpl::OnInterfaceRequestForFrame(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  registry_.TryBindInterface(interface_name, interface_pipe);
}

void ElectronApiServiceImpl::DidCreateDocumentElement() {
  document_created_ = true;

  if (pending_receiver_) {
    if (receiver_.is_bound())
      receiver_.reset();

    receiver_.Bind(std::move(pending_receiver_));
    receiver_.set_disconnect_handler(base::BindOnce(
        &ElectronApiServiceImpl::OnConnectionError, GetWeakPtr()));
  }
}

void ElectronApiServiceImpl::OnDestruct() {
  delete this;
}

void ElectronApiServiceImpl::OnConnectionError() {
  if (receiver_.is_bound())
    receiver_.reset();
}

void ElectronApiServiceImpl::Message(bool internal,
                                     const std::string& channel,
                                     electron::SerializedValue arguments) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Value> args = gin::ConvertToV8(isolate, arguments);

  ipc_native::EmitIPCEvent(isolate, context, internal, channel, {}, args);
}

void ElectronApiServiceImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Value> message_value = DeserializeV8Value(isolate, message);

  std::vector<v8::Local<v8::Value>> ports;
  for (auto& port : message.ports) {
    ports.emplace_back(
        blink::WebMessagePortConverter::EntangleAndInjectMessagePortChannel(
            isolate, context, std::move(port)));
  }

  std::vector<v8::Local<v8::Value>> args = {message_value};

  ipc_native::EmitIPCEvent(isolate, context, false, channel, ports,
                           gin::ConvertToV8(isolate, args));
}

void ElectronApiServiceImpl::TakeHeapSnapshot(
    mojo::ScopedHandle file,
    TakeHeapSnapshotCallback callback) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  ScopedAllowBlockingForElectron allow_blocking;

  base::ScopedPlatformFile platform_file;
  if (mojo::UnwrapPlatformFile(std::move(file), &platform_file) !=
      MOJO_RESULT_OK) {
    LOG(ERROR) << "Unable to get the file handle from mojo.";
    std::move(callback).Run(false);
    return;
  }
  base::File base_file(std::move(platform_file));

  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  bool success = electron::TakeHeapSnapshot(isolate, &base_file);

  std::move(callback).Run(success);
}

void ElectronApiServiceImpl::ExecuteJavaScript(
    int32_t world_id,
    std::vector<mojom::ScriptSourcePtr> sources,
    bool has_user_gesture,
    ExecuteJavaScriptCallback callback) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  std::vector<blink::WebScriptSource> web_sources;
  web_sources.reserve(sources.size());
  for (const auto& source : sources) {
    web_sources.emplace_back(blink::WebString::FromUtf16(source->code),
                             blink::WebURL(GURL(source->url)));
  }
  auto* reply = new ScriptReply(std::move(callback));
  frame->RequestExecuteScript(
      world_id, web_sources,
      has_user_gesture ? blink::mojom::UserActivationOption::kActivate
                       : blink::mojom::UserActivationOption::kDoNotActivate,
      blink::mojom::EvaluationTiming::kSynchronous,
      blink::mojom::LoadEventBlockingOption::kDoNotBlock, base::NullCallback(),
      base::BindOnce(&ScriptReply::Completed, base::Unretained(reply)),
      blink::BackForwardCacheAware::kAllow,
      blink::mojom::WantResultOption::kWantResult,
      blink::mojom::PromiseResultOption::kDoNotWait,
      /*is_injected_extension_script=*/false);
}

void ElectronApiServiceImpl::InsertCSS(const std::string& css,
                                       const std::string& css_origin,
                                       InsertCSSCallback callback) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  std::move(callback).Run(
      frame->GetDocument()
          .InsertStyleSheet(blink::WebString::FromUtf8(css), nullptr,
                            css_origin == "user" ? blink::WebCssOrigin::kUser
                                                 : blink::WebCssOrigin::kAuthor)
          .Utf16());
}

void ElectronApiServiceImpl::RemoveInsertedCSS(
    const std::u16string& key,
    RemoveInsertedCSSCallback callback) {
  render_frame()->GetWebFrame()->GetDocument().RemoveInsertedStyleSheet(
      blink::WebString::FromUtf16(key));
  std::move(callback).Run();
}

void ElectronApiServiceImpl::InsertText(const std::string& text,
                                        InsertTextCallback callback) {
  render_frame()
      ->GetWebFrame()
      ->FrameWidget()
      ->GetActiveWebInputMethodController()
      ->CommitText(blink::WebString::FromUtf8(text),
                   std::vector<ui::ImeTextSpan>(), blink::WebRange(), 0);
  std::move(callback).Run();
}

void ElectronApiServiceImpl::SetVisualZoomLevelLimits(
    double min_level,
    double max_level,
    SetVisualZoomLevelLimitsCallback callback) {
  render_frame()->GetWebFrame()->View()->SetDefaultPageScaleLimits(min_level,
                                                                   max_level);
  std::move(callback).Run();
}

}  // namespace electron
