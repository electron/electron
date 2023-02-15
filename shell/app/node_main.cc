// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/node_main.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/content_switches.h"
#include "electron/electron_version.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "shell/app/uv_task_runner.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/child/v8_crashpad_support_win.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "base/environment.h"
#include "base/posix/global_descriptors.h"
#include "base/strings/string_number_conversions.h"
#include "components/crash/core/app/crash_switches.h"  // nogncheck
#include "content/public/common/content_descriptors.h"
#endif

#if !defined(MAS_BUILD)
#include "components/crash/core/app/crashpad.h"  // nogncheck
#include "shell/app/electron_crash_reporter_client.h"
#include "shell/common/crash_keys.h"
#endif

namespace {

// Initialize Node.js cli options to pass to Node.js
// See https://nodejs.org/api/cli.html#cli_options
int SetNodeCliFlags() {
  // Options that are unilaterally disallowed
  const std::unordered_set<base::StringPiece, base::StringPieceHash>
      disallowed = {"--openssl-config", "--use-bundled-ca", "--use-openssl-ca",
                    "--force-fips", "--enable-fips"};

  const auto argv = base::CommandLine::ForCurrentProcess()->argv();
  std::vector<std::string> args;

  // TODO(codebytere): We need to set the first entry in args to the
  // process name owing to src/node_options-inl.h#L286-L290 but this is
  // redundant and so should be refactored upstream.
  args.reserve(argv.size() + 1);
  args.emplace_back("electron");

  for (const auto& arg : argv) {
#if BUILDFLAG(IS_WIN)
    const auto& option = base::WideToUTF8(arg);
#else
    const auto& option = arg;
#endif
    const auto stripped = base::StringPiece(option).substr(0, option.find('='));
    if (disallowed.count(stripped) != 0) {
      LOG(ERROR) << "The Node.js cli flag " << stripped
                 << " is not supported in Electron";
      // Node.js returns 9 from ProcessGlobalArgs for any errors encountered
      // when setting up cli flags and env vars. Since we're outlawing these
      // flags (making them errors) return 9 here for consistency.
      return 9;
    } else {
      args.push_back(option);
    }
  }

  std::vector<std::string> errors;

  // Node.js itself will output parsing errors to
  // console so we don't need to handle that ourselves
  return ProcessGlobalArgs(&args, nullptr, &errors,
                           node::kDisallowedInEnvironment);
}

#if defined(MAS_BUILD)
void SetCrashKeyStub(const std::string& key, const std::string& value) {}
void ClearCrashKeyStub(const std::string& key) {}
#endif

}  // namespace

namespace electron {

v8::Local<v8::Value> GetParameters(v8::Isolate* isolate) {
  std::map<std::string, std::string> keys;
#if !defined(MAS_BUILD)
  electron::crash_keys::GetCrashKeys(&keys);
#endif
  return gin::ConvertToV8(isolate, keys);
}

int NodeMain(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);

#if BUILDFLAG(IS_WIN)
  v8_crashpad_support::SetUp();
#endif

#if BUILDFLAG(IS_LINUX)
  auto os_env = base::Environment::Create();
  std::string fd_string, pid_string;
  if (os_env->GetVar("CRASHDUMP_SIGNAL_FD", &fd_string) &&
      os_env->GetVar("CRASHPAD_HANDLER_PID", &pid_string)) {
    int fd = -1, pid = -1;
    DCHECK(base::StringToInt(fd_string, &fd));
    DCHECK(base::StringToInt(pid_string, &pid));
    base::GlobalDescriptors::GetInstance()->Set(kCrashDumpSignal, fd);
    // Following API is unsafe in multi-threaded scenario, but at this point
    // we are still single threaded.
    os_env->UnSetVar("CRASHDUMP_SIGNAL_FD");
    os_env->UnSetVar("CRASHPAD_HANDLER_PID");
  }
#endif

  int exit_code = 1;
  {
    // Feed gin::PerIsolateData with a task runner.
    uv_loop_t* loop = uv_default_loop();
    auto uv_task_runner = base::MakeRefCounted<UvTaskRunner>(loop);
    base::ThreadTaskRunnerHandle handle(uv_task_runner);

    // Initialize feature list.
    auto feature_list = std::make_unique<base::FeatureList>();
    feature_list->InitializeFromCommandLine("", "");
    base::FeatureList::SetInstance(std::move(feature_list));

    // Explicitly register electron's builtin modules.
    NodeBindings::RegisterBuiltinModules();

    // Parse and set Node.js cli flags.
    int flags_exit_code = SetNodeCliFlags();
    if (flags_exit_code != 0)
      exit(flags_exit_code);

    node::InitializationSettingsFlags flags = node::kRunPlatformInit;
    node::InitializationResult result =
        node::InitializeOncePerProcess(argc, argv, flags);

    if (result.early_return)
      exit(result.exit_code);

#if BUILDFLAG(IS_LINUX)
    // On Linux, initialize crashpad after Nodejs init phase so that
    // crash and termination signal handlers can be set by the crashpad client.
    if (!pid_string.empty()) {
      auto* command_line = base::CommandLine::ForCurrentProcess();
      command_line->AppendSwitchASCII(
          crash_reporter::switches::kCrashpadHandlerPid, pid_string);
      ElectronCrashReporterClient::Create();
      crash_reporter::InitializeCrashpad(false, "node");
      crash_keys::SetCrashKeysFromCommandLine(
          *base::CommandLine::ForCurrentProcess());
      crash_keys::SetPlatformCrashKey();
      // Ensure the flags and env variable does not propagate to userland.
      command_line->RemoveSwitch(crash_reporter::switches::kCrashpadHandlerPid);
    }
#elif BUILDFLAG(IS_WIN) || (BUILDFLAG(IS_MAC) && !defined(MAS_BUILD))
    ElectronCrashReporterClient::Create();
    crash_reporter::InitializeCrashpad(false, "node");
    crash_keys::SetCrashKeysFromCommandLine(
        *base::CommandLine::ForCurrentProcess());
    crash_keys::SetPlatformCrashKey();
#endif

    gin::V8Initializer::LoadV8Snapshot(
        gin::V8SnapshotFileType::kWithAdditionalContext);

    // V8 requires a task scheduler.
    base::ThreadPoolInstance::CreateAndStartWithDefaultParams("Electron");

    // Allow Node.js to track the amount of time the event loop has spent
    // idle in the kernel’s event provider .
    uv_loop_configure(loop, UV_METRICS_IDLE_TIME);

    // Initialize gin::IsolateHolder.
    JavascriptEnvironment gin_env(loop);

    v8::Isolate* isolate = gin_env.isolate();

    v8::Isolate::Scope isolate_scope(isolate);
    v8::Locker locker(isolate);
    node::Environment* env = nullptr;
    node::IsolateData* isolate_data = nullptr;
    {
      v8::HandleScope scope(isolate);

      isolate_data = node::CreateIsolateData(isolate, loop, gin_env.platform());
      CHECK_NE(nullptr, isolate_data);

      uint64_t env_flags = node::EnvironmentFlags::kDefaultFlags |
                           node::EnvironmentFlags::kHideConsoleWindows;
      env = node::CreateEnvironment(
          isolate_data, gin_env.context(), result.args, result.exec_args,
          static_cast<node::EnvironmentFlags::Flags>(env_flags));
      CHECK_NE(nullptr, env);

      node::IsolateSettings is;
      node::SetIsolateUpForNode(isolate, is);

      gin_helper::Dictionary process(isolate, env->process_object());
      process.SetMethod("crash", &ElectronBindings::Crash);

      // Setup process.crashReporter in child node processes
      gin_helper::Dictionary reporter = gin::Dictionary::CreateEmpty(isolate);
      reporter.SetMethod("getParameters", &GetParameters);
#if defined(MAS_BUILD)
      reporter.SetMethod("addExtraParameter", &SetCrashKeyStub);
      reporter.SetMethod("removeExtraParameter", &ClearCrashKeyStub);
#else
      reporter.SetMethod("addExtraParameter",
                         &electron::crash_keys::SetCrashKey);
      reporter.SetMethod("removeExtraParameter",
                         &electron::crash_keys::ClearCrashKey);
#endif

      process.Set("crashReporter", reporter);

      gin_helper::Dictionary versions;
      if (process.Get("versions", &versions)) {
        versions.SetReadOnly(ELECTRON_PROJECT_NAME, ELECTRON_VERSION_STRING);
      }
    }

    v8::HandleScope scope(isolate);
    node::LoadEnvironment(env, node::StartExecutionCallback{});

    // Potential reasons we get Nothing here may include: the env
    // is stopping, or the user hooks process.emit('exit').
    exit_code = node::SpinEventLoop(env).FromMaybe(1);

    node::ResetStdio();

    node::Stop(env);
    node::FreeEnvironment(env);
    node::FreeIsolateData(isolate_data);
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
