// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_utility_process.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/no_destructor.h"
#include "base/process/kill.h"
#include "base/process/process.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/common/child_process_host.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "shell/browser/api/message_port.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_value_serializer.h"

namespace electron {

base::IDMap<api::UtilityProcessWrapper*>& GetAllUtilityProcessWrappers() {
  static base::NoDestructor<base::IDMap<api::UtilityProcessWrapper*>>
      s_all_utility_process_wrappers;
  return *s_all_utility_process_wrappers;
}

namespace api {

gin::WrapperInfo UtilityProcessWrapper::kWrapperInfo = {
    gin::kEmbedderNativeGin};

UtilityProcessWrapper::UtilityProcessWrapper(
    node::mojom::NodeServiceParamsPtr params,
    std::u16string display_name,
    bool use_plugin_helper)
    : wrapper_id_(GetAllUtilityProcessWrappers().Add(this)) {
  DCHECK(!node_service_remote_.is_bound());

  node_service_remote_.reset();
  mojo::PendingReceiver<node::mojom::NodeService> receiver =
      node_service_remote_.BindNewPipeAndPassReceiver();

  content::ServiceProcessHost::Launch(
      std::move(receiver),
      content::ServiceProcessHost::Options()
          .WithDisplayName(display_name.empty()
                               ? std::u16string(u"Node Service")
                               : display_name)
          .WithExtraCommandLineSwitches(params->exec_args)
#if BUILDFLAG(IS_MAC)
          .WithChildFlags(use_plugin_helper
                              ? content::ChildProcessHost::CHILD_PLUGIN
                              : content::ChildProcessHost::CHILD_NORMAL)
#endif
          .WithProcessCallback(
              base::BindOnce(&UtilityProcessWrapper::OnServiceProcessLaunched,
                             weak_factory_.GetWeakPtr()))
          .Pass());
  node_service_remote_.set_disconnect_with_reason_handler(
      base::BindOnce(&UtilityProcessWrapper::OnServiceProcessDisconnected,
                     weak_factory_.GetWeakPtr()));
  node_service_remote_->Initialize(std::move(params));
}

UtilityProcessWrapper::~UtilityProcessWrapper() = default;

void UtilityProcessWrapper::OnServiceProcessLaunched(
    const base::Process& process) {
  DCHECK(node_service_remote_.is_connected());
  pid_ = process.Pid();
  // Emit 'spawn' event
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> self;
  if (GetWrapper(isolate).ToLocal(&self))
    gin_helper::EmitEvent(isolate, self, "spawn");
}

void UtilityProcessWrapper::OnServiceProcessDisconnected(
    uint32_t error_code,
    const std::string& description) {
  if (GetAllUtilityProcessWrappers().Lookup(wrapper_id_))
    GetAllUtilityProcessWrappers().Remove(wrapper_id_);
  node_service_remote_.reset();
  // Emit 'exit' event
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> self;
  if (GetWrapper(isolate).ToLocal(&self)) {
    gin_helper::EmitEvent(isolate, self, "exit");
    pinned_wrapper_.Reset();
  }
}

void UtilityProcessWrapper::ShutdownServiceProcess() {
  node_service_remote_.reset();
  // Emit 'exit' event
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> self;
  if (GetWrapper(isolate).ToLocal(&self)) {
    gin_helper::EmitEvent(isolate, self, "exit");
    pinned_wrapper_.Reset();
  }
}

void UtilityProcessWrapper::Pin(v8::Isolate* isolate) {
  // Prevent ourselves from being GC'd until the process is exited.
  pinned_wrapper_.Reset(isolate, GetWrapper(isolate).ToLocalChecked());
}

void UtilityProcessWrapper::PostMessage(gin::Arguments* args) {
  if (!node_service_remote_.is_connected())
    return;

  blink::TransferableMessage transferable_message;
  std::string channel;
  if (!args->GetNext(&channel)) {
    args->ThrowTypeError("Expected at least one argument to postMessage");
    return;
  }

  v8::Local<v8::Value> message_value;
  if (args->GetNext(&message_value)) {
    if (!electron::SerializeV8Value(args->isolate(), message_value,
                                    &transferable_message)) {
      // SerializeV8Value sets an exception.
      return;
    }
  }

  v8::Local<v8::Value> transferables;
  std::vector<gin::Handle<MessagePort>> wrapped_ports;
  if (args->GetNext(&transferables)) {
    if (!gin::ConvertFromV8(args->isolate(), transferables, &wrapped_ports)) {
      args->isolate()->ThrowException(v8::Exception::Error(
          gin::StringToV8(args->isolate(), "Invalid value for transfer")));
      return;
    }
  }

  bool threw_exception = false;
  transferable_message.ports = MessagePort::DisentanglePorts(
      args->isolate(), wrapped_ports, &threw_exception);
  if (threw_exception)
    return;

  node_service_remote_->ReceivePostMessage(channel,
                                           std::move(transferable_message));
}

int UtilityProcessWrapper::Kill(int signal) const {
  if (pid_ == base::kNullProcessId)
    return 0;
  return uv_kill(pid_, signal);
}

v8::Local<v8::Value> UtilityProcessWrapper::GetOSProcessId(
    v8::Isolate* isolate) const {
  if (pid_ == base::kNullProcessId)
    return v8::Undefined(isolate);
  return gin::ConvertToV8(isolate, pid_);
}

// static
gin::Handle<UtilityProcessWrapper> UtilityProcessWrapper::Create(
    gin::Arguments* args) {
  gin_helper::Dictionary opts;
  if (!args->GetNext(&opts)) {
    args->ThrowTypeError("Expected a dictionary");
    return gin::Handle<UtilityProcessWrapper>();
  }
  node::mojom::NodeServiceParamsPtr params =
      node::mojom::NodeServiceParams::New();
  opts.Get("modulePath", &params->script);
  opts.Get("args", &params->args);
  std::map<std::string, std::string> env_map;
  if (opts.Get("env", &env_map)) {
    for (auto const& env : env_map) {
      params->environment.push_back(
          node::mojom::EnvironmentVariable::New(env.first, env.second));
    }
  }
  opts.Get("execArgv", &params->exec_args);
  std::u16string display_name;
  opts.Get("serviceName", &display_name);
  bool use_plugin_helper = false;
#if BUILDFLAG(IS_MAC)
  opts.Get("disableLibraryValidation", &use_plugin_helper);
#endif
  auto handle = gin::CreateHandle(
      args->isolate(), new UtilityProcessWrapper(
                           std::move(params), display_name, use_plugin_helper));
  handle->Pin(args->isolate());
  return handle;
}

// static
gin::ObjectTemplateBuilder UtilityProcessWrapper::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
             UtilityProcessWrapper>::GetObjectTemplateBuilder(isolate)
      .SetMethod("postMessage", &UtilityProcessWrapper::PostMessage)
      .SetMethod("kill", &UtilityProcessWrapper::Kill)
      .SetProperty("pid", &UtilityProcessWrapper::GetOSProcessId);
}

const char* UtilityProcessWrapper::GetTypeName() {
  return "UtilityProcessWrapper";
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("CreateProcessWrapper",
                 &electron::api::UtilityProcessWrapper::Create);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_utility_process, Initialize)
