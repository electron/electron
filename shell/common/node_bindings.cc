// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_bindings.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_paths.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/mac/main_application_bundle.h"
#include "shell/common/node_includes.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_initializer.h"  // nogncheck

#if !defined(MAS_BUILD)
#include "shell/common/crash_keys.h"
#endif

#define ELECTRON_BUILTIN_MODULES(V)      \
  V(electron_browser_app)                \
  V(electron_browser_auto_updater)       \
  V(electron_browser_browser_view)       \
  V(electron_browser_content_tracing)    \
  V(electron_browser_crash_reporter)     \
  V(electron_browser_dialog)             \
  V(electron_browser_event)              \
  V(electron_browser_event_emitter)      \
  V(electron_browser_global_shortcut)    \
  V(electron_browser_in_app_purchase)    \
  V(electron_browser_menu)               \
  V(electron_browser_message_port)       \
  V(electron_browser_net)                \
  V(electron_browser_power_monitor)      \
  V(electron_browser_power_save_blocker) \
  V(electron_browser_protocol)           \
  V(electron_browser_printing)           \
  V(electron_browser_safe_storage)       \
  V(electron_browser_session)            \
  V(electron_browser_system_preferences) \
  V(electron_browser_base_window)        \
  V(electron_browser_tray)               \
  V(electron_browser_view)               \
  V(electron_browser_web_contents)       \
  V(electron_browser_web_contents_view)  \
  V(electron_browser_web_frame_main)     \
  V(electron_browser_web_view_manager)   \
  V(electron_browser_window)             \
  V(electron_common_asar)                \
  V(electron_common_clipboard)           \
  V(electron_common_command_line)        \
  V(electron_common_environment)         \
  V(electron_common_features)            \
  V(electron_common_native_image)        \
  V(electron_common_native_theme)        \
  V(electron_common_notification)        \
  V(electron_common_screen)              \
  V(electron_common_shell)               \
  V(electron_common_v8_util)             \
  V(electron_renderer_context_bridge)    \
  V(electron_renderer_crash_reporter)    \
  V(electron_renderer_ipc)               \
  V(electron_renderer_web_frame)

#define ELECTRON_VIEWS_MODULES(V) V(electron_browser_image_view)

#define ELECTRON_DESKTOP_CAPTURER_MODULE(V) V(electron_browser_desktop_capturer)

#define ELECTRON_TESTING_MODULE(V) V(electron_common_testing)

// This is used to load built-in modules. Instead of using
// __attribute__((constructor)), we call the _register_<modname>
// function for each built-in modules explicitly. This is only
// forward declaration. The definitions are in each module's
// implementation when calling the NODE_LINKED_MODULE_CONTEXT_AWARE.
#define V(modname) void _register_##modname();
ELECTRON_BUILTIN_MODULES(V)
#if BUILDFLAG(ENABLE_VIEWS_API)
ELECTRON_VIEWS_MODULES(V)
#endif
#if BUILDFLAG(ENABLE_DESKTOP_CAPTURER)
ELECTRON_DESKTOP_CAPTURER_MODULE(V)
#endif
#if DCHECK_IS_ON()
ELECTRON_TESTING_MODULE(V)
#endif
#undef V

namespace {

void stop_and_close_uv_loop(uv_loop_t* loop) {
  uv_stop(loop);

  auto const ensure_closing = [](uv_handle_t* handle, void*) {
    // We should be using the UvHandle wrapper everywhere, in which case
    // all handles should already be in a closing state...
    DCHECK(uv_is_closing(handle));
    // ...but if a raw handle got through, through, do the right thing anyway
    if (!uv_is_closing(handle)) {
      uv_close(handle, nullptr);
    }
  };

  uv_walk(loop, ensure_closing, nullptr);

  // All remaining handles are in a closing state now.
  // Pump the event loop so that they can finish closing.
  for (;;)
    if (uv_run(loop, UV_RUN_DEFAULT) == 0)
      break;

  DCHECK_EQ(0, uv_loop_alive(loop));
}

bool g_is_initialized = false;

void V8FatalErrorCallback(const char* location, const char* message) {
  LOG(ERROR) << "Fatal error in V8: " << location << " " << message;

#if !defined(MAS_BUILD)
  electron::crash_keys::SetCrashKey("electron.v8-fatal.message", message);
  electron::crash_keys::SetCrashKey("electron.v8-fatal.location", location);
#endif

  volatile int* zero = nullptr;
  *zero = 0;
}

bool AllowWasmCodeGenerationCallback(v8::Local<v8::Context> context,
                                     v8::Local<v8::String>) {
  // If we're running with contextIsolation enabled in the renderer process,
  // fall back to Blink's logic.
  v8::Isolate* isolate = context->GetIsolate();
  if (node::Environment::GetCurrent(isolate) == nullptr) {
    if (gin_helper::Locker::IsBrowserProcess())
      return false;
    return blink::V8Initializer::WasmCodeGenerationCheckCallbackInMainThread(
        context, v8::String::Empty(isolate));
  }

  return node::AllowWasmCodeGenerationCallback(context,
                                               v8::String::Empty(isolate));
}

void ErrorMessageListener(v8::Local<v8::Message> message,
                          v8::Local<v8::Value> data) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  gin_helper::MicrotasksScope microtasks_scope(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
  node::Environment* env = node::Environment::GetCurrent(isolate);

  if (env) {
    // Emit the after() hooks now that the exception has been handled.
    // Analogous to node/lib/internal/process/execution.js#L176-L180
    if (env->async_hooks()->fields()[node::AsyncHooks::kAfter]) {
      while (env->async_hooks()->fields()[node::AsyncHooks::kStackLength]) {
        node::AsyncWrap::EmitAfter(env, env->execution_async_id());
        env->async_hooks()->pop_async_context(env->execution_async_id());
      }
    }

    // Ensure that the async id stack is properly cleared so the async
    // hook stack does not become corrupted.
    env->async_hooks()->clear_async_id_stack();
  }
}

const std::unordered_set<base::StringPiece, base::StringPieceHash>
GetAllowedDebugOptions() {
  if (electron::fuses::IsNodeCliInspectEnabled()) {
    // Only allow DebugOptions in non-ELECTRON_RUN_AS_NODE mode
    return {
        "--inspect",          "--inspect-brk",
        "--inspect-port",     "--debug",
        "--debug-brk",        "--debug-port",
        "--inspect-brk-node", "--inspect-publish-uid",
    };
  }
  // If node CLI inspect support is disabled, allow no debug options.
  return {};
}

// Initialize Node.js cli options to pass to Node.js
// See https://nodejs.org/api/cli.html#cli_options
void SetNodeCliFlags() {
  const std::unordered_set<base::StringPiece, base::StringPieceHash> allowed =
      GetAllowedDebugOptions();

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

    // Only allow in no-op (--) option or DebugOptions
    if (allowed.count(stripped) != 0 || stripped == "--")
      args.push_back(option);
  }

  std::vector<std::string> errors;
  const int exit_code = ProcessGlobalArgs(&args, nullptr, &errors,
                                          node::kDisallowedInEnvironment);

  const std::string err_str = "Error parsing Node.js cli flags ";
  if (exit_code != 0) {
    LOG(ERROR) << err_str;
  } else if (!errors.empty()) {
    LOG(ERROR) << err_str << base::JoinString(errors, " ");
  }
}

// Initialize NODE_OPTIONS to pass to Node.js
// See https://nodejs.org/api/cli.html#cli_node_options_options
void SetNodeOptions(base::Environment* env) {
  // Options that are unilaterally disallowed
  const std::set<std::string> disallowed = {
      "--openssl-config", "--use-bundled-ca", "--use-openssl-ca",
      "--force-fips", "--enable-fips"};

  // Subset of options allowed in packaged apps
  const std::set<std::string> allowed_in_packaged = {"--max-http-header-size",
                                                     "--http-parser"};

  if (env->HasVar("NODE_OPTIONS")) {
    if (electron::fuses::IsNodeOptionsEnabled()) {
      std::string options;
      env->GetVar("NODE_OPTIONS", &options);
      std::vector<std::string> parts = base::SplitString(
          options, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

      bool is_packaged_app = electron::api::App::IsPackaged();

      for (const auto& part : parts) {
        // Strip off values passed to individual NODE_OPTIONs
        std::string option = part.substr(0, part.find('='));

        if (is_packaged_app &&
            allowed_in_packaged.find(option) == allowed_in_packaged.end()) {
          // Explicitly disallow majority of NODE_OPTIONS in packaged apps
          LOG(ERROR) << "Most NODE_OPTIONs are not supported in packaged apps."
                     << " See documentation for more details.";
          options.erase(options.find(option), part.length());
        } else if (disallowed.find(option) != disallowed.end()) {
          // Remove NODE_OPTIONS specifically disallowed for use in Node.js
          // through Electron owing to constraints like BoringSSL.
          LOG(ERROR) << "The NODE_OPTION " << option
                     << " is not supported in Electron";
          options.erase(options.find(option), part.length());
        }
      }

      // overwrite new NODE_OPTIONS without unsupported variables
      env->SetVar("NODE_OPTIONS", options);
    } else {
      LOG(ERROR) << "NODE_OPTIONS have been disabled in this app";
      env->SetVar("NODE_OPTIONS", "");
    }
  }
}

}  // namespace

namespace electron {

namespace {

base::FilePath GetResourcesPath() {
#if BUILDFLAG(IS_MAC)
  return MainApplicationBundlePath().Append("Contents").Append("Resources");
#else
  auto* command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath exec_path(command_line->GetProgram());
  base::PathService::Get(base::FILE_EXE, &exec_path);

  return exec_path.DirName().Append(FILE_PATH_LITERAL("resources"));
#endif
}

}  // namespace

NodeBindings::NodeBindings(BrowserEnvironment browser_env)
    : browser_env_(browser_env) {
  if (browser_env == BrowserEnvironment::kWorker) {
    uv_loop_init(&worker_loop_);
    uv_loop_ = &worker_loop_;
  } else {
    uv_loop_ = uv_default_loop();
  }
}

NodeBindings::~NodeBindings() {
  // Quit the embed thread.
  embed_closed_ = true;
  uv_sem_post(&embed_sem_);

  WakeupEmbedThread();

  // Wait for everything to be done.
  uv_thread_join(&embed_thread_);

  // Clear uv.
  uv_sem_destroy(&embed_sem_);
  dummy_uv_handle_.reset();

  // Clean up worker loop
  if (in_worker_loop())
    stop_and_close_uv_loop(uv_loop_);
}

void NodeBindings::RegisterBuiltinModules() {
#define V(modname) _register_##modname();
  ELECTRON_BUILTIN_MODULES(V)
#if BUILDFLAG(ENABLE_VIEWS_API)
  ELECTRON_VIEWS_MODULES(V)
#endif
#if BUILDFLAG(ENABLE_DESKTOP_CAPTURER)
  ELECTRON_DESKTOP_CAPTURER_MODULE(V)
#endif
#if DCHECK_IS_ON()
  ELECTRON_TESTING_MODULE(V)
#endif
#undef V
}

bool NodeBindings::IsInitialized() {
  return g_is_initialized;
}

void NodeBindings::Initialize() {
  TRACE_EVENT0("electron", "NodeBindings::Initialize");
  // Open node's error reporting system for browser process.
  node::g_upstream_node_mode = false;

#if BUILDFLAG(IS_LINUX)
  // Get real command line in renderer process forked by zygote.
  if (browser_env_ != BrowserEnvironment::kBrowser)
    ElectronCommandLine::InitializeFromCommandLine();
#endif

  // Explicitly register electron's builtin modules.
  RegisterBuiltinModules();

  // Parse and set Node.js cli flags.
  SetNodeCliFlags();

  auto env = base::Environment::Create();
  SetNodeOptions(env.get());
  node::Environment::should_read_node_options_from_env_ =
      fuses::IsNodeOptionsEnabled();

  std::vector<std::string> argv = {"electron"};
  std::vector<std::string> exec_argv;
  std::vector<std::string> errors;

  int exit_code = node::InitializeNodeWithArgs(&argv, &exec_argv, &errors);

  for (const std::string& error : errors)
    fprintf(stderr, "%s: %s\n", argv[0].c_str(), error.c_str());

  if (exit_code != 0)
    exit(exit_code);

#if BUILDFLAG(IS_WIN)
  // uv_init overrides error mode to suppress the default crash dialog, bring
  // it back if user wants to show it.
  if (browser_env_ == BrowserEnvironment::kBrowser ||
      env->HasVar("ELECTRON_DEFAULT_ERROR_MODE"))
    SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);
#endif

  g_is_initialized = true;
}

node::Environment* NodeBindings::CreateEnvironment(
    v8::Handle<v8::Context> context,
    node::MultiIsolatePlatform* platform) {
#if BUILDFLAG(IS_WIN)
  auto& atom_args = ElectronCommandLine::argv();
  std::vector<std::string> args(atom_args.size());
  std::transform(atom_args.cbegin(), atom_args.cend(), args.begin(),
                 [](auto& a) { return base::WideToUTF8(a); });
#else
  auto args = ElectronCommandLine::argv();
#endif

  // Feed node the path to initialization script.
  std::string process_type;
  switch (browser_env_) {
    case BrowserEnvironment::kBrowser:
      process_type = "browser";
      break;
    case BrowserEnvironment::kRenderer:
      process_type = "renderer";
      break;
    case BrowserEnvironment::kWorker:
      process_type = "worker";
      break;
  }

  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary global(isolate, context->Global());
  // Do not set DOM globals for renderer process.
  // We must set this before the node bootstrapper which is run inside
  // CreateEnvironment
  if (browser_env_ != BrowserEnvironment::kBrowser)
    global.Set("_noBrowserGlobals", true);

  if (browser_env_ == BrowserEnvironment::kBrowser) {
    const std::vector<std::string> search_paths = {"app.asar", "app",
                                                   "default_app.asar"};
    const std::vector<std::string> app_asar_search_paths = {"app.asar"};
    context->Global()->SetPrivate(
        context,
        v8::Private::ForApi(
            isolate,
            gin::ConvertToV8(isolate, "appSearchPaths").As<v8::String>()),
        gin::ConvertToV8(isolate,
                         electron::fuses::IsOnlyLoadAppFromAsarEnabled()
                             ? app_asar_search_paths
                             : search_paths));
  }

  std::vector<std::string> exec_args;
  base::FilePath resources_path = GetResourcesPath();
  std::string init_script = "electron/js2c/" + process_type + "_init";

  args.insert(args.begin() + 1, init_script);

  if (!isolate_data_)
    isolate_data_ = node::CreateIsolateData(isolate, uv_loop_, platform);

  node::Environment* env;
  uint64_t flags = node::EnvironmentFlags::kDefaultFlags |
                   node::EnvironmentFlags::kHideConsoleWindows |
                   node::EnvironmentFlags::kNoGlobalSearchPaths;

  if (browser_env_ != BrowserEnvironment::kBrowser) {
    // Only one ESM loader can be registered per isolate -
    // in renderer processes this should be blink. We need to tell Node.js
    // not to register its handler (overriding blinks) in non-browser processes.
    flags |= node::EnvironmentFlags::kNoRegisterESMLoader |
             node::EnvironmentFlags::kNoInitializeInspector;
    v8::TryCatch try_catch(context->GetIsolate());
    env = node::CreateEnvironment(
        isolate_data_, context, args, exec_args,
        static_cast<node::EnvironmentFlags::Flags>(flags));
    DCHECK(env);

    // This will only be caught when something has gone terrible wrong as all
    // electron scripts are wrapped in a try {} catch {} by webpack
    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Failed to initialize node environment in process: "
                 << process_type;
    }
  } else {
    env = node::CreateEnvironment(
        isolate_data_, context, args, exec_args,
        static_cast<node::EnvironmentFlags::Flags>(flags));
    DCHECK(env);
  }

  // Clean up the global _noBrowserGlobals that we unironically injected into
  // the global scope
  if (browser_env_ != BrowserEnvironment::kBrowser) {
    // We need to bootstrap the env in non-browser processes so that
    // _noBrowserGlobals is read correctly before we remove it
    global.Delete("_noBrowserGlobals");
  }

  node::IsolateSettings is;

  // Use a custom fatal error callback to allow us to add
  // crash message and location to CrashReports.
  is.fatal_error_callback = V8FatalErrorCallback;

  // We don't want to abort either in the renderer or browser processes.
  // We already listen for uncaught exceptions and handle them there.
  is.should_abort_on_uncaught_exception_callback = [](v8::Isolate*) {
    return false;
  };

  // Use a custom callback here to allow us to leverage Blink's logic in the
  // renderer process.
  is.allow_wasm_code_generation_callback = AllowWasmCodeGenerationCallback;

  if (browser_env_ == BrowserEnvironment::kBrowser) {
    // Node.js requires that microtask checkpoints be explicitly invoked.
    is.policy = v8::MicrotasksPolicy::kExplicit;
  } else {
    // Blink expects the microtasks policy to be kScoped, but Node.js expects it
    // to be kExplicit. In the renderer, there can be many contexts within the
    // same isolate, so we don't want to change the existing policy here, which
    // could be either kExplicit or kScoped depending on whether we're executing
    // from within a Node.js or a Blink entrypoint. Instead, the policy is
    // toggled to kExplicit when entering Node.js through UvRunOnce.
    is.policy = context->GetIsolate()->GetMicrotasksPolicy();

    // We do not want to use Node.js' message listener as it interferes with
    // Blink's.
    is.flags &= ~node::IsolateSettingsFlags::MESSAGE_LISTENER_WITH_ERROR_LEVEL;

    // Isolate message listeners are additive (you can add multiple), so instead
    // we add an extra one here to ensure that the async hook stack is properly
    // cleared when errors are thrown.
    context->GetIsolate()->AddMessageListenerWithErrorLevel(
        ErrorMessageListener, v8::Isolate::kMessageError);

    // We do not want to use the promise rejection callback that Node.js uses,
    // because it does not send PromiseRejectionEvents to the global script
    // context. We need to use the one Blink already provides.
    is.flags |=
        node::IsolateSettingsFlags::SHOULD_NOT_SET_PROMISE_REJECTION_CALLBACK;

    // We do not want to use the stack trace callback that Node.js uses,
    // because it relies on Node.js being aware of the current Context and
    // that's not always the case. We need to use the one Blink already
    // provides.
    is.flags |=
        node::IsolateSettingsFlags::SHOULD_NOT_SET_PREPARE_STACK_TRACE_CALLBACK;
  }

  node::SetIsolateUpForNode(context->GetIsolate(), is);

  gin_helper::Dictionary process(context->GetIsolate(), env->process_object());
  process.SetReadOnly("type", process_type);
  process.Set("resourcesPath", resources_path);
  // The path to helper app.
  base::FilePath helper_exec_path;
  base::PathService::Get(content::CHILD_PROCESS_EXE, &helper_exec_path);
  process.Set("helperExecPath", helper_exec_path);

  return env;
}

void NodeBindings::LoadEnvironment(node::Environment* env) {
  node::LoadEnvironment(env, node::StartExecutionCallback{});
  gin_helper::EmitEvent(env->isolate(), env->process_object(), "loaded");
}

void NodeBindings::PrepareMessageLoop() {
  // Add dummy handle for libuv, otherwise libuv would quit when there is
  // nothing to do.
  uv_async_init(uv_loop_, dummy_uv_handle_.get(), nullptr);

  // Start worker that will interrupt main loop when having uv events.
  uv_sem_init(&embed_sem_, 0);
  uv_thread_create(&embed_thread_, EmbedThreadRunner, this);
}

void NodeBindings::RunMessageLoop() {
  // The MessageLoop should have been created, remember the one in main thread.
  task_runner_ = base::ThreadTaskRunnerHandle::Get();

  // Run uv loop for once to give the uv__io_poll a chance to add all events.
  UvRunOnce();
}

void NodeBindings::UvRunOnce() {
  node::Environment* env = uv_env();

  // When doing navigation without restarting renderer process, it may happen
  // that the node environment is destroyed but the message loop is still there.
  // In this case we should not run uv loop.
  if (!env)
    return;

  // Use Locker in browser process.
  gin_helper::Locker locker(env->isolate());
  v8::HandleScope handle_scope(env->isolate());

  // Enter node context while dealing with uv events.
  v8::Context::Scope context_scope(env->context());

  // Node.js expects `kExplicit` microtasks policy and will run microtasks
  // checkpoints after every call into JavaScript. Since we use a different
  // policy in the renderer - switch to `kExplicit` and then drop back to the
  // previous policy value.
  auto old_policy = env->isolate()->GetMicrotasksPolicy();
  DCHECK_EQ(v8::MicrotasksScope::GetCurrentDepth(env->isolate()), 0);
  env->isolate()->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);

  if (browser_env_ != BrowserEnvironment::kBrowser)
    TRACE_EVENT_BEGIN0("devtools.timeline", "FunctionCall");

  // Deal with uv events.
  int r = uv_run(uv_loop_, UV_RUN_NOWAIT);

  if (browser_env_ != BrowserEnvironment::kBrowser)
    TRACE_EVENT_END0("devtools.timeline", "FunctionCall");

  env->isolate()->SetMicrotasksPolicy(old_policy);

  if (r == 0)
    base::RunLoop().QuitWhenIdle();  // Quit from uv.

  // Tell the worker thread to continue polling.
  uv_sem_post(&embed_sem_);
}

void NodeBindings::WakeupMainThread() {
  DCHECK(task_runner_);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&NodeBindings::UvRunOnce,
                                                   weak_factory_.GetWeakPtr()));
}

void NodeBindings::WakeupEmbedThread() {
  uv_async_send(dummy_uv_handle_.get());
}

// static
void NodeBindings::EmbedThreadRunner(void* arg) {
  auto* self = static_cast<NodeBindings*>(arg);

  while (true) {
    // Wait for the main loop to deal with events.
    uv_sem_wait(&self->embed_sem_);
    if (self->embed_closed_)
      break;

    // Wait for something to happen in uv loop.
    // Note that the PollEvents() is implemented by derived classes, so when
    // this class is being destructed the PollEvents() would not be available
    // anymore. Because of it we must make sure we only invoke PollEvents()
    // when this class is alive.
    self->PollEvents();
    if (self->embed_closed_)
      break;

    // Deal with event in main thread.
    self->WakeupMainThread();
  }
}

}  // namespace electron
