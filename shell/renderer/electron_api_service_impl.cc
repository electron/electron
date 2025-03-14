// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/electron_api_service_impl.h"

#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "base/trace_event/trace_event.h"
#include "gin/data_object_builder.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/heap_snapshot.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/common/thread_restrictions.h"
#include "shell/common/v8_util.h"
#include "shell/renderer/electron_ipc_native.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/renderer_client_base.h"
#include "third_party/blink/public/mojom/frame/user_activation_notification_type.mojom-shared.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_message_port_converter.h"

namespace electron {

ElectronApiServiceImpl::~ElectronApiServiceImpl() = default;

ElectronApiServiceImpl::ElectronApiServiceImpl(
    content::RenderFrame* render_frame,
    RendererClientBase* renderer_client)
    : content::RenderFrameObserver(render_frame),
      renderer_client_(renderer_client) {
  registry_.AddInterface<mojom::ElectronRenderer>(base::BindRepeating(
      &ElectronApiServiceImpl::BindTo, base::Unretained(this)));
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
                                     blink::CloneableMessage arguments) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Value> args = gin::ConvertToV8(isolate, arguments);

  ipc_native::EmitIPCEvent(context, internal, channel, {}, args);
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
            context, std::move(port)));
  }

  std::vector<v8::Local<v8::Value>> args = {message_value};

  ipc_native::EmitIPCEvent(context, false, channel, ports,
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

}  // namespace electron
