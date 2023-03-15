// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/services/node/node_service.h"

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_switches.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/services/node/parent_port.h"

#if BUILDFLAG(USE_PARTITION_ALLOC_AS_MALLOC)
#include "base/allocator/partition_alloc_support.h"
#include "base/allocator/partition_allocator/partition_address_space.h"
#endif

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
  if (!node_env_stopped_) {
    node_env_->env()->set_trace_sync_io(false);
    js_env_->DestroyMicrotasksRunner();
    node::Stop(node_env_->env(), false);
  }
}

void NodeService::Initialize(node::mojom::NodeServiceParamsPtr params) {
  if (NodeBindings::IsInitialized())
    return;

  ParentPort::GetInstance()->Initialize(std::move(params->port));

  js_env_ = std::make_unique<JavascriptEnvironment>(node_bindings_->uv_loop());

#if BUILDFLAG(USE_PARTITION_ALLOC_AS_MALLOC)
  // Gin will initialize the partition alloc configurable pool
  // according to the V8 Sandbox preallocated addressspace in
  // gin::V8Initializer::Initialize. Given there can only be a single
  // configurable pool per process, we will now reroute the main partition
  // allocation into this new pool below. The pool size is currently limited to
  // 16GiB which will now be shared by array buffer allocations as well as other
  // heap allocations from the process. We can increase the pool size if need
  // arises or create a secondary configurable pool.
  CHECK_NE(v8::V8::GetSandboxAddressSpace(), nullptr);
  CHECK(partition_alloc::IsConfigurablePoolAvailable());
  auto* cmd = base::CommandLine::ForCurrentProcess();
  std::string process_type = cmd->GetSwitchValueASCII(switches::kProcessType);
  if (params->use_v8_configured_pool_for_main_partition) {
    base::allocator::PartitionAllocSupport::Get()
        ->ReconfigureAfterFeatureListInit(
            process_type, true /* configure_dangling_pointer_detector */,
            true /* use_configured_pool */);
    base::allocator::PartitionAllocSupport::Get()
        ->ReconfigureAfterTaskRunnerInit(process_type);
  } else {
    base::allocator::PartitionAllocSupport::Get()
        ->ReconfigureAfterFeatureListInit(process_type);
    base::allocator::PartitionAllocSupport::Get()
        ->ReconfigureAfterTaskRunnerInit(process_type);
  }
#endif  // BUILDFLAG(USE_PARTITION_ALLOC_AS_MALLOC)

  v8::HandleScope scope(js_env_->isolate());

  node_bindings_->Initialize(js_env_->isolate()->GetCurrentContext());

  // Append program path for process.argv0
  auto program = base::CommandLine::ForCurrentProcess()->GetProgram();
#if defined(OS_WIN)
  params->args.insert(params->args.begin(), base::WideToUTF8(program.value()));
#else
  params->args.insert(params->args.begin(), program.value());
#endif

  // Create the global environment.
  node::Environment* env = node_bindings_->CreateEnvironment(
      js_env_->isolate()->GetCurrentContext(), js_env_->platform(),
      params->args, params->exec_args);
  node_env_ = std::make_unique<NodeEnvironment>(env);

  node::SetProcessExitHandler(env,
                              [this](node::Environment* env, int exit_code) {
                                // Destroy node platform.
                                env->set_trace_sync_io(false);
                                js_env_->DestroyMicrotasksRunner();
                                node::Stop(env, false);
                                node_env_stopped_ = true;
                                receiver_.ResetWithReason(exit_code, "");
                              });

  env->set_trace_sync_io(env->options()->trace_sync_io);

  // Add Electron extended APIs.
  electron_bindings_->BindTo(env->isolate(), env->process_object());

  // Add entry script to process object.
  gin_helper::Dictionary process(env->isolate(), env->process_object());
  process.SetHidden("_serviceStartupScript", params->script);

  // Setup microtask runner.
  js_env_->CreateMicrotasksRunner();

  // Wrap the uv loop with global env.
  node_bindings_->set_uv_env(env);

  // LoadEnvironment should be called after setting up
  // JavaScriptEnvironment including the microtask runner
  // since this call will start compilation and execution
  // of the entry script. If there is an uncaught exception
  // the exit handler set above will be triggered and it expects
  // both Node Env and JavaScriptEnviroment are setup to perform
  // a clean shutdown of this process.
  node_bindings_->LoadEnvironment(env);

  // Run entry script.
  node_bindings_->PrepareEmbedThread();
  node_bindings_->StartPolling();
}

}  // namespace electron
