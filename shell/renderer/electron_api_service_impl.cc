// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/electron_api_service_impl.h"

#include <tuple>
#include <utility>
#include <vector>

#include "gin/converter.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "shell/common/api/electron_api_shared_texture.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/serialized_value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/heap_snapshot.h"
#include "shell/common/thread_restrictions.h"
#include "shell/common/v8_util.h"
#include "shell/renderer/electron_ipc_native.h"
#include "shell/renderer/preload_utils.h"

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
#include "v8/include/v8-function.h"

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

// Replies to an ExecuteJavaScript() call with the script's completion value
// (Blink has already awaited it if it was a thenable) or with what it threw.
// Failures that have no value to report go back as a message so that nothing
// here touches V8 when Blink calls back while tearing the context down.
void ReplyWithScriptResult(
    mojom::ElectronFrame::ExecuteJavaScriptCallback callback,
    const std::vector<v8::Local<v8::Value>>& results,
    const std::vector<v8::Local<v8::Value>>& rejections) {
  if (results.empty()) {
    std::move(callback).Run(
        false, electron::SerializedValue(),
        "WebFrame was removed before script could run. This normally means "
        "the underlying frame was destroyed");
    return;
  }
  bool success = false;
  v8::Local<v8::Value> value;
  if (!rejections.empty() && !rejections[0].IsEmpty()) {
    value = rejections[0];
  } else if (results[0].IsEmpty()) {
    std::move(callback).Run(
        false, electron::SerializedValue(),
        "Script failed to execute, this normally means an error was thrown. "
        "Check the renderer console for the error.");
    return;
  } else {
    success = true;
    value = results[0];
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::TryCatch try_catch(isolate);
  electron::SerializedValue serialized;
  if (!electron::SerializeV8Value(isolate, value, &serialized)) {
    std::string error = "An object could not be cloned.";
    v8::Local<v8::Value> message;
    if (try_catch.HasCaught() && !try_catch.HasTerminated() &&
        try_catch.Exception()->IsObject() &&
        try_catch.Exception()
            .As<v8::Object>()
            ->Get(isolate->GetCurrentContext(),
                  gin::StringToSymbol(isolate, "message"))
            .ToLocal(&message) &&
        message->IsString()) {
      gin::ConvertFromV8(isolate, message, &error);
    }
    std::move(callback).Run(false, electron::SerializedValue(), error);
    return;
  }
  std::move(callback).Run(success, std::move(serialized), std::string());
}

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
  frame->RequestExecuteScript(
      world_id, web_sources,
      has_user_gesture ? blink::mojom::UserActivationOption::kActivate
                       : blink::mojom::UserActivationOption::kDoNotActivate,
      blink::mojom::EvaluationTiming::kSynchronous,
      blink::mojom::LoadEventBlockingOption::kDoNotBlock, base::NullCallback(),
      base::BindOnce(&ReplyWithScriptResult, std::move(callback)),
      blink::BackForwardCacheAware::kAllow,
      blink::mojom::WantResultOption::kWantResult,
      blink::mojom::PromiseResultOption::kAwait,
      /*script_injector_id=*/blink::WebString());
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

namespace {

// release() of the object handed to the shared texture receiver: releases the
// imported texture, then tells the browser this frame no longer references it.
void ReleaseReceivedSharedTexture(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Array> data = info.Data().As<v8::Array>();
  v8::Local<v8::Value> imported, texture_id, release;
  if (!data->Get(context, 0).ToLocal(&imported) || !imported->IsObject() ||
      !data->Get(context, 1).ToLocal(&texture_id) ||
      !imported.As<v8::Object>()
           ->Get(context, gin::StringToSymbol(isolate, "release"))
           .ToLocal(&release) ||
      !release->IsFunction()) {
    return;
  }
  v8::Local<v8::Function> notify_browser;
  if (!v8::Function::New(
           context,
           [](const v8::FunctionCallbackInfo<v8::Value>& info) {
             v8::Isolate* isolate = info.GetIsolate();
             v8::Local<v8::Context> context = isolate->GetCurrentContext();
             v8::Local<v8::Value> binding = preload_utils::GetBinding(
                 isolate, gin::StringToV8(isolate, "electron_renderer_ipc"));
             v8::Local<v8::Value> ipc_renderer, invoke;
             if (binding.IsEmpty() || !binding->IsObject() ||
                 !binding.As<v8::Object>()
                      ->Get(context,
                            gin::StringToSymbol(isolate, "ipcRenderer"))
                      .ToLocal(&ipc_renderer) ||
                 !ipc_renderer->IsObject() ||
                 !ipc_renderer.As<v8::Object>()
                      ->Get(context, gin::StringToSymbol(isolate, "invoke"))
                      .ToLocal(&invoke) ||
                 !invoke->IsFunction()) {
               return;
             }
             v8::Local<v8::Value> args[] = {
                 gin::StringToV8(
                     isolate, "IMPORT_SHARED_TEXTURE_RELEASE_RENDERER_TO_MAIN"),
                 info.Data()};
             v8::Local<v8::Value> result;
             if (invoke.As<v8::Function>()
                     ->Call(context, ipc_renderer, 2, args)
                     .ToLocal(&result)) {
               info.GetReturnValue().Set(result);
             }
           },
           texture_id, 0, v8::ConstructorBehavior::kThrow)
           .ToLocal(&notify_browser)) {
    return;
  }
  v8::Local<v8::Value> argv[] = {notify_browser};
  v8::Local<v8::Value> result;
  if (release.As<v8::Function>()
          ->Call(context, imported, 1, argv)
          .ToLocal(&result)) {
    info.GetReturnValue().Set(result);
  }
}

}  // namespace

void ElectronApiServiceImpl::ReceiveSharedTexture(
    electron::SerializedValue transfer,
    const std::string& texture_id,
    electron::SerializedValue args,
    ReceiveSharedTextureCallback callback) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(context,
                                       v8::MicrotasksScope::kRunMicrotasks);
  v8::TryCatch try_catch(isolate);

  auto reply_error = [&]() {
    v8::Local<v8::Value> error =
        try_catch.HasCaught()
            ? try_catch.Exception()
            : v8::Exception::Error(
                  gin::StringToV8(isolate, "Failed to import shared texture"));
    try_catch.Reset();
    electron::SerializedValue serialized;
    if (!electron::SerializeV8Value(isolate, error, &serialized)) {
      std::move(callback).Run(false, electron::SerializedValue(),
                              "Failed to import shared texture");
      return;
    }
    std::move(callback).Run(false, std::move(serialized), std::string());
  };

  auto get = [&](v8::Local<v8::Value> object, const char* key,
                 v8::Local<v8::Value>* out) {
    return object->IsObject() &&
           object.As<v8::Object>()
               ->Get(context, gin::StringToSymbol(isolate, key))
               .ToLocal(out);
  };

  // imported = sharedTexture.subtle.finishTransferSharedTexture({...transfer,
  // id})
  v8::Local<v8::Value> transfer_value = gin::ConvertToV8(isolate, transfer);
  if (!transfer_value->IsObject() ||
      transfer_value.As<v8::Object>()
          ->Set(context, gin::StringToSymbol(isolate, "id"),
                gin::StringToV8(isolate, texture_id))
          .IsNothing()) {
    return reply_error();
  }
  v8::Local<v8::Value> imported =
      api::shared_texture::FinishTransferSharedTexture(isolate, transfer_value);
  if (try_catch.HasCaught() || imported.IsEmpty() || !imported->IsObject())
    return reply_error();

  // Reply with imported.getFrameCreationSyncToken().
  v8::Local<v8::Value> get_sync_token, sync_token;
  electron::SerializedValue serialized_token;
  if (!get(imported, "getFrameCreationSyncToken", &get_sync_token) ||
      !get_sync_token->IsFunction() ||
      !get_sync_token.As<v8::Function>()
           ->Call(context, imported, 0, nullptr)
           .ToLocal(&sync_token) ||
      !electron::SerializeV8Value(isolate, sync_token, &serialized_token)) {
    return reply_error();
  }
  std::move(callback).Run(true, std::move(serialized_token), std::string());

  // receiver({ importedSharedTexture: { textureId, subtle, getVideoFrame,
  // release } }, ...args)
  try_catch.SetVerbose(true);
  gin_helper::Dictionary global(isolate, context->Global());
  v8::Local<v8::Value> receiver;
  if (!global.GetHidden("sharedTextureReceiver", &receiver) ||
      !receiver->IsFunction()) {
    return;
  }
  v8::Local<v8::Value> get_video_frame;
  if (!get(imported, "getVideoFrame", &get_video_frame))
    return;
  v8::Local<v8::Value> release_data_items[] = {
      imported, gin::StringToV8(isolate, texture_id)};
  v8::Local<v8::Function> release;
  if (!v8::Function::New(context, ReleaseReceivedSharedTexture,
                         v8::Array::New(isolate, release_data_items, 2), 0,
                         v8::ConstructorBehavior::kThrow)
           .ToLocal(&release)) {
    return;
  }
  auto wrapper = gin_helper::Dictionary::CreateEmpty(isolate);
  wrapper.Set("textureId", texture_id);
  wrapper.Set("subtle", imported);
  wrapper.Set("getVideoFrame", get_video_frame);
  wrapper.Set("release", release.As<v8::Value>());
  auto data = gin_helper::Dictionary::CreateEmpty(isolate);
  data.Set("importedSharedTexture", wrapper);

  v8::LocalVector<v8::Value> argv(isolate, {data.GetHandle()});
  v8::Local<v8::Value> extra = gin::ConvertToV8(isolate, args);
  if (extra->IsArray()) {
    v8::Local<v8::Array> extra_array = extra.As<v8::Array>();
    for (uint32_t i = 0; i < extra_array->Length(); ++i) {
      v8::Local<v8::Value> item;
      if (extra_array->Get(context, i).ToLocal(&item))
        argv.push_back(item);
    }
  }
  std::ignore = receiver.As<v8::Function>()->Call(
      context, v8::Undefined(isolate), static_cast<int>(argv.size()),
      argv.data());
}

}  // namespace electron
