// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/services/node/node_service.h"

#include <utility>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "shell/browser/api/message_port.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/v8_value_serializer.h"

namespace electron {

NodeService::NodeService(
    mojo::PendingReceiver<node::mojom::NodeService> receiver)
    : node_bindings_(
          NodeBindings::Create(NodeBindings::BrowserEnvironment::kUtility)),
      electron_bindings_(
          std::make_unique<ElectronBindings>(node_bindings_->uv_loop())) {
  if (receiver.is_valid())
    receiver_.Bind(std::move(receiver));
}

NodeService::~NodeService() {
  // Destroy node platform.
  node_env_->env()->set_trace_sync_io(false);
  js_env_->OnMessageLoopDestroying();
  node::Stop(node_env_->env());
}

void NodeService::Initialize(node::mojom::NodeServiceParamsPtr params) {
  if (initialized_)
    return;

  initialized_ = true;

  if (!params->environment.empty()) {
    std::unique_ptr<base::Environment> env(base::Environment::Create());
    for (const auto& variable : params->environment) {
      env->SetVar(variable->name, variable->value);
    }
  }

  js_env_ = std::make_unique<JavascriptEnvironment>(node_bindings_->uv_loop());

  v8::HandleScope scope(js_env_->isolate());

  node_bindings_->Initialize();

  // Append program path for process.argv0
  auto program = base::CommandLine::ForCurrentProcess()->GetProgram();
#if defined(OS_WIN)
  params->args.insert(params->args.begin(), base::WideToUTF8(program.value()));
#else
  params->args.insert(params->args.begin(), program.value());
#endif

  // Create the global environment.
  node::Environment* env = node_bindings_->CreateEnvironment(
      js_env_->context(), js_env_->platform(), params->args, params->exec_args);
  node_env_ = std::make_unique<NodeEnvironment>(env);

  env->set_trace_sync_io(env->options()->trace_sync_io);

  // Add Electron extended APIs.
  electron_bindings_->BindTo(env->isolate(), env->process_object());

  // Add entry script to process object.
  gin_helper::Dictionary process(env->isolate(), env->process_object());
  process.SetReadOnly("_serviceStartupScript", params->script);

  // Load everything.
  node_bindings_->LoadEnvironment(env);

  // Wrap the uv loop with global env.
  node_bindings_->set_uv_env(env);

  // Setup microtask runner.
  js_env_->OnMessageLoopCreated();

  // Run entry script.
  node_bindings_->PrepareEmbedThread();
  node_bindings_->StartPolling();
}

void NodeService::ReceivePostMessage(const std::string& channel,
                                     blink::TransferableMessage message) {
  v8::Isolate* isolate = js_env_->isolate();
  v8::HandleScope handle_scope(isolate);
  auto wrapped_ports =
      MessagePort::EntanglePorts(isolate, std::move(message.ports));
  v8::Local<v8::Value> message_value =
      electron::DeserializeV8Value(isolate, message);

  auto event =
      gin::DataObjectBuilder(isolate).Set("data", message_value).Build();

  gin_helper::EmitEvent(isolate, node_env_->env()->process_object(),
                        "-ipc-ports", event, channel, std::move(wrapped_ports));
}

}  // namespace electron
