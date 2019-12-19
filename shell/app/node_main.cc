// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/node_main.h"

#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread_task_runner_handle.h"
#include "electron/electron_version.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "shell/app/uv_task_runner.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/node_debugger.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/crash_reporter/crash_reporter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"

#if defined(_WIN64)
#include "shell/common/crash_reporter/crash_reporter_win.h"
#endif

namespace electron {

#if !defined(OS_LINUX)
void AddExtraParameter(const std::string& key, const std::string& value) {
  crash_reporter::CrashReporter::GetInstance()->AddExtraParameter(key, value);
}

void RemoveExtraParameter(const std::string& key) {
  crash_reporter::CrashReporter::GetInstance()->RemoveExtraParameter(key);
}
#endif

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

    gin_helper::Dictionary process(gin_env.isolate(), env->process_object());
#if defined(OS_WIN)
    process.SetMethod("log", &ElectronBindings::Log);
#endif
    process.SetMethod("crash", &ElectronBindings::Crash);

    // Setup process.crashReporter.start in child node processes
    gin_helper::Dictionary reporter =
        gin::Dictionary::CreateEmpty(gin_env.isolate());
    reporter.SetMethod("start", &crash_reporter::CrashReporter::StartInstance);

#if !defined(OS_LINUX)
    reporter.SetMethod("addExtraParameter", &AddExtraParameter);
    reporter.SetMethod("removeExtraParameter", &RemoveExtraParameter);
#endif

    process.Set("crashReporter", reporter);

    gin_helper::Dictionary versions;
    if (process.Get("versions", &versions)) {
      versions.SetReadOnly(ELECTRON_PROJECT_NAME, ELECTRON_VERSION_STRING);
    }

    node::LoadEnvironment(env);
    v8::Isolate* isolate = env->isolate();

    {
      v8::SealHandleScope seal(isolate);
      bool more;
      do {
        uv_run(env->event_loop(), UV_RUN_DEFAULT);

        gin_env.platform()->DrainTasks(env->isolate());

        more = uv_loop_alive(env->event_loop());
        if (more && !env->is_stopping())
          continue;

        if (!uv_loop_alive(env->event_loop())) {
          EmitBeforeExit(env);
        }

        // Emit `beforeExit` if the loop became alive either after emitting
        // event, or after running some callbacks.
        more = uv_loop_alive(env->event_loop());
      } while (more && !env->is_stopping());
    }

    node_debugger.Stop();
    exit_code = node::EmitExit(env);

    node::ResetStdio();

    env->set_can_call_into_js(false);
    env->stop_sub_worker_contexts();
    env->RunCleanup();

    node::RunAtExit(env);
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

}  // namespace electron
