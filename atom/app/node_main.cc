// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/node_main.h"

#include "atom/app/uv_task_runner.h"
#include "atom/browser/javascript_environment.h"
#include "atom/browser/node_debugger.h"
#include "atom/common/node_includes.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/thread_task_runner_handle.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"

namespace atom {

int NodeMain(int argc, char *argv[]) {
  base::CommandLine::Init(argc, argv);

  int exit_code = 1;
  {
    // Feed gin::PerIsolateData with a task runner.
    argv = uv_setup_args(argc, argv);
    uv_loop_t* loop = uv_default_loop();
    scoped_refptr<UvTaskRunner> uv_task_runner(new UvTaskRunner(loop));
    base::ThreadTaskRunnerHandle handle(uv_task_runner);

    // Initialize feature list.
    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->InitializeFromCommandLine("", "");
    base::FeatureList::SetInstance(std::move(feature_list));

    gin::V8Initializer::LoadV8Snapshot();
    gin::V8Initializer::LoadV8Natives();
    JavascriptEnvironment gin_env;

    int exec_argc;
    const char** exec_argv;
    node::Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

    node::Environment* env = node::CreateEnvironment(
        gin_env.isolate(), loop, gin_env.context(), argc, argv,
        exec_argc, exec_argv);

    // Start our custom debugger implementation.
    NodeDebugger node_debugger(gin_env.isolate());
    if (node_debugger.IsRunning())
      env->AssignToContext(v8::Debug::GetDebugContext());

    node::LoadEnvironment(env);

    bool more;
    do {
      more = uv_run(env->event_loop(), UV_RUN_ONCE);
      if (more == false) {
        node::EmitBeforeExit(env);

        // Emit `beforeExit` if the loop became alive either after emitting
        // event, or after running some callbacks.
        more = uv_loop_alive(env->event_loop());
        if (uv_run(env->event_loop(), UV_RUN_NOWAIT) != 0)
          more = true;
      }
    } while (more == true);

    exit_code = node::EmitExit(env);
    node::RunAtExit(env);

    env->Dispose();
  }

  v8::V8::Dispose();

  return exit_code;
}

}  // namespace atom
