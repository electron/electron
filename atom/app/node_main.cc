// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/node_main.h"

#include <memory>
#include <utility>

#include "atom/app/uv_task_runner.h"
#include "atom/browser/javascript_environment.h"
#include "atom/browser/node_debugger.h"
#include "atom/common/api/electron_bindings.h"
#include "atom/common/atom_version.h"
#include "atom/common/crash_reporter/crash_reporter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/task/thread_pool/thread_pool.h"
#include "base/threading/thread_task_runner_handle.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "native_mate/dictionary.h"

#if defined(_WIN64)
#include "atom/common/crash_reporter/crash_reporter_win.h"
#endif

namespace atom {

int NodeMain(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);

  int exit_code = 1;
  {
    // Feed gin::PerIsolateData with a task runner.
    argv = uv_setup_args(argc, argv);
    uv_loop_t* loop = uv_default_loop();
    scoped_refptr<UvTaskRunner> uv_task_runner(new UvTaskRunner(loop));
    base::ThreadTaskRunnerHandle handle(uv_task_runner);

    // Initialize feature list.
    auto feature_list = std::make_unique<base::FeatureList>();
    feature_list->InitializeFromCommandLine("", "");
    base::FeatureList::SetInstance(std::move(feature_list));

#if defined(_WIN64)
    crash_reporter::CrashReporterWin::SetUnhandledExceptionFilter();
#endif

    // Explicitly register electron's builtin modules.
    NodeBindings::RegisterBuiltinModules();

    int exec_argc;
    const char** exec_argv;
    node::Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

    gin::V8Initializer::LoadV8Snapshot(
        gin::V8Initializer::V8SnapshotFileType::kWithAdditionalContext);
    gin::V8Initializer::LoadV8Natives();

    // V8 requires a task scheduler apparently
    base::ThreadPoolInstance::CreateAndStartWithDefaultParams("Electron");

    // Initialize gin::IsolateHolder.
    JavascriptEnvironment gin_env(loop);

    node::IsolateData* isolate_data =
        node::CreateIsolateData(gin_env.isolate(), loop, gin_env.platform());
    CHECK_NE(nullptr, isolate_data);

    node::Environment* env =
        node::CreateEnvironment(isolate_data, gin_env.context(), argc, argv,
                                exec_argc, exec_argv, false);
    CHECK_NE(nullptr, env);

    // Enable support for v8 inspector.
    NodeDebugger node_debugger(env);
    node_debugger.Start();

    node::BootstrapEnvironment(env);

    mate::Dictionary process(gin_env.isolate(), env->process_object());
#if defined(OS_WIN)
    process.SetMethod("log", &ElectronBindings::Log);
#endif
    process.SetMethod("crash", &ElectronBindings::Crash);

    // Setup process.crashReporter.start in child node processes
    auto reporter = mate::Dictionary::CreateEmpty(gin_env.isolate());
    reporter.SetMethod("start", &crash_reporter::CrashReporter::StartInstance);
    process.Set("crashReporter", reporter);

    mate::Dictionary versions;
    if (process.Get("versions", &versions)) {
      versions.SetReadOnly(ATOM_PROJECT_NAME, ATOM_VERSION_STRING);
    }

    node::LoadEnvironment(env);

    bool more;
    do {
      more = uv_run(env->event_loop(), UV_RUN_ONCE);
      gin_env.platform()->DrainTasks(env->isolate());
      if (more == false) {
        node::EmitBeforeExit(env);

        // Emit `beforeExit` if the loop became alive either after emitting
        // event, or after running some callbacks.
        more = uv_loop_alive(env->event_loop());
        if (uv_run(env->event_loop(), UV_RUN_NOWAIT) != 0)
          more = true;
      }
    } while (more == true);

    node_debugger.Stop();
    exit_code = node::EmitExit(env);
    env->set_can_call_into_js(false);
    node::RunAtExit(env);

    v8::Isolate* isolate = env->isolate();
    node::FreeEnvironment(env);
    node::FreeIsolateData(isolate_data);

    gin_env.platform()->DrainTasks(isolate);
    gin_env.platform()->CancelPendingDelayedTasks(isolate);
    gin_env.platform()->UnregisterIsolate(isolate);
  }

  // According to "src/gin/shell/gin_main.cc":
  //
  // gin::IsolateHolder waits for tasks running in ThreadPool in its
  // destructor and thus must be destroyed before ThreadPool starts skipping
  // CONTINUE_ON_SHUTDOWN tasks.
  base::ThreadPoolInstance::Get()->Shutdown();

  v8::V8::Dispose();

  return exit_code;
}

}  // namespace atom
