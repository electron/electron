// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/node_main.h"

#include "atom/browser/javascript_environment.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"

#include "atom/common/node_includes.h"

namespace atom {

int NodeMain(int argc, char *argv[]) {
  argv = uv_setup_args(argc, argv);
  int exec_argc;
  const char** exec_argv;
  node::Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

  int exit_code = 1;
  {
    gin::V8Initializer::LoadV8Snapshot();
    gin::IsolateHolder::Initialize(
        gin::IsolateHolder::kNonStrictMode,
        gin::ArrayBufferAllocator::SharedInstance());

    JavascriptEnvironment gin_env;
    node::Environment* env = node::CreateEnvironment(
        gin_env.isolate(), uv_default_loop(), gin_env.context(), argc, argv,
        exec_argc, exec_argv);

    // Start debugger.
    node::node_isolate = gin_env.isolate();
    if (node::use_debug_agent)
      node::StartDebug(env, node::debug_wait_connect);

    node::LoadEnvironment(env);

    // Enable debugger.
    if (node::use_debug_agent)
      node::EnableDebug(env);

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
