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
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_paths.h"
#include "electron/buildflags/buildflags.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/mac/main_application_bundle.h"
#include "shell/common/node_includes.h"

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
  V(electron_browser_session)            \
  V(electron_browser_system_preferences) \
  V(electron_browser_top_level_window)   \
  V(electron_browser_tray)               \
  V(electron_browser_view)               \
  V(electron_browser_web_contents)       \
  V(electron_browser_web_contents_view)  \
  V(electron_browser_web_view_manager)   \
  V(electron_browser_window)             \
  V(electron_common_asar)                \
  V(electron_common_clipboard)           \
  V(electron_common_command_line)        \
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
#undef V

namespace {

void stop_and_close_uv_loop(uv_loop_t* loop) {
  // Close any active handles
  uv_stop(loop);
  uv_walk(
      loop,
      [](uv_handle_t* handle, void*) {
        if (!uv_is_closing(handle)) {
          uv_close(handle, nullptr);
        }
      },
      nullptr);

  // Run the loop to let it finish all the closing handles
  // NB: after uv_stop(), uv_run(UV_RUN_DEFAULT) returns 0 when that's done
  for (;;)
    if (!uv_run(loop, UV_RUN_DEFAULT))
      break;

  DCHECK(!uv_loop_alive(loop));
  uv_loop_close(loop);
}

bool g_is_initialized = false;

bool IsPackagedApp() {
  base::FilePath exe_path;
  base::PathService::Get(base::FILE_EXE, &exe_path);
  base::FilePath::StringType base_name =
      base::ToLowerASCII(exe_path.BaseName().value());

#if defined(OS_WIN)
  return base_name != FILE_PATH_LITERAL("electron.exe");
#else
  return base_name != FILE_PATH_LITERAL("electron");
#endif
}

// Initialize Node.js cli options to pass to Node.js
// See https://nodejs.org/api/cli.html#cli_options
void SetNodeCliFlags() {
  // Only allow DebugOptions in non-ELECTRON_RUN_AS_NODE mode
  const std::unordered_set<base::StringPiece, base::StringPieceHash> allowed = {
      "--inspect",          "--inspect-brk",
      "--inspect-port",     "--debug",
      "--debug-brk",        "--debug-port",
      "--inspect-brk-node", "--inspect-publish-uid",
  };

  const auto argv = base::CommandLine::ForCurrentProcess()->argv();
  std::vector<std::string> args;

  // TODO(codebytere): We need to set the first entry in args to the
  // process name owing to src/node_options-inl.h#L286-L290 but this is
  // redundant and so should be refactored upstream.
  args.reserve(argv.size() + 1);
  args.emplace_back("electron");

  for (const auto& arg : argv) {
#if defined(OS_WIN)
    const auto& option = base::UTF16ToUTF8(arg);
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
}  // namespace

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
    std::string options;
    env->GetVar("NODE_OPTIONS", &options);
    std::vector<std::string> parts = base::SplitString(
        options, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    bool is_packaged_app = IsPackagedApp();

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
  }
}

bool AllowWasmCodeGenerationCallback(v8::Local<v8::Context> context,
                                     v8::Local<v8::String>) {
  v8::Local<v8::Value> wasm_code_gen = context->GetEmbedderData(
      node::ContextEmbedderIndex::kAllowWasmCodeGeneration);
  return wasm_code_gen->IsUndefined() || wasm_code_gen->IsTrue();
}

}  // namespace

namespace electron {

namespace {

// Convert the given vector to an array of C-strings. The strings in the
// returned vector are only guaranteed valid so long as the vector of strings
// is not modified.
std::unique_ptr<const char* []> StringVectorToArgArray(
    const std::vector<std::string>& vector) {
  std::unique_ptr<const char*[]> array(new const char*[vector.size()]);
  for (size_t i = 0; i < vector.size(); ++i) {
    array[i] = vector[i].c_str();
  }
  return array;
}

base::FilePath GetResourcesPath() {
#if defined(OS_MACOSX)
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
    : browser_env_(browser_env), weak_factory_(this) {
  if (browser_env == BrowserEnvironment::WORKER) {
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
  uv_close(reinterpret_cast<uv_handle_t*>(&dummy_uv_handle_), nullptr);

  // Clean up worker loop
  if (uv_loop_ == &worker_loop_)
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
#undef V
}

bool NodeBindings::IsInitialized() {
  return g_is_initialized;
}

void NodeBindings::Initialize() {
  TRACE_EVENT0("electron", "NodeBindings::Initialize");
  // Open node's error reporting system for browser process.
  node::g_standalone_mode = browser_env_ == BrowserEnvironment::BROWSER;
  node::g_upstream_node_mode = false;

#if defined(OS_LINUX)
  // Get real command line in renderer process forked by zygote.
  if (browser_env_ != BrowserEnvironment::BROWSER)
    ElectronCommandLine::InitializeFromCommandLine();
#endif

  // Explicitly register electron's builtin modules.
  RegisterBuiltinModules();

  // Parse and set Node.js cli flags.
  SetNodeCliFlags();

  // pass non-null program name to argv so it doesn't crash
  // trying to index into a nullptr
  int argc = 1;
  int exec_argc = 0;
  const char* prog_name = "electron";
  const char** argv = &prog_name;
  const char** exec_argv = nullptr;

  std::unique_ptr<base::Environment> env(base::Environment::Create());
  SetNodeOptions(env.get());

  // TODO(codebytere): this is going to be deprecated in the near future
  // in favor of Init(std::vector<std::string>* argv,
  //        std::vector<std::string>* exec_argv)
  node::Init(&argc, argv, &exec_argc, &exec_argv);

#if defined(OS_WIN)
  // uv_init overrides error mode to suppress the default crash dialog, bring
  // it back if user wants to show it.
  if (browser_env_ == BrowserEnvironment::BROWSER ||
      env->HasVar("ELECTRON_DEFAULT_ERROR_MODE"))
    SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);
#endif

  g_is_initialized = true;
}

node::Environment* NodeBindings::CreateEnvironment(
    v8::Handle<v8::Context> context,
    node::MultiIsolatePlatform* platform) {
#if defined(OS_WIN)
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
    case BrowserEnvironment::BROWSER:
      process_type = "browser";
      break;
    case BrowserEnvironment::RENDERER:
      process_type = "renderer";
      break;
    case BrowserEnvironment::WORKER:
      process_type = "worker";
      break;
  }

  gin_helper::Dictionary global(context->GetIsolate(), context->Global());
  // Do not set DOM globals for renderer process.
  // We must set this before the node bootstrapper which is run inside
  // CreateEnvironment
  if (browser_env_ != BrowserEnvironment::BROWSER)
    global.Set("_noBrowserGlobals", true);

  base::FilePath resources_path = GetResourcesPath();
  std::string init_script = "electron/js2c/" + process_type + "_init";

  args.insert(args.begin() + 1, init_script);

  std::unique_ptr<const char*[]> c_argv = StringVectorToArgArray(args);
  isolate_data_ =
      node::CreateIsolateData(context->GetIsolate(), uv_loop_, platform);
  node::Environment* env = node::CreateEnvironment(
      isolate_data_, context, args.size(), c_argv.get(), 0, nullptr);
  DCHECK(env);

  // Clean up the global _noBrowserGlobals that we unironically injected into
  // the global scope
  if (browser_env_ != BrowserEnvironment::BROWSER) {
    // We need to bootstrap the env in non-browser processes so that
    // _noBrowserGlobals is read correctly before we remove it
    global.Delete("_noBrowserGlobals");
  }

  if (browser_env_ == BrowserEnvironment::BROWSER) {
    // This policy requires that microtask checkpoints be explicitly invoked.
    // Node.js requires this.
    context->GetIsolate()->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
  } else {
    // Match Blink's behavior by allowing microtasks invocation to be controlled
    // by MicrotasksScope objects.
    context->GetIsolate()->SetMicrotasksPolicy(v8::MicrotasksPolicy::kScoped);
  }

  // This needs to be called before the inspector is initialized.
  env->InitializeDiagnostics();

  // Set the callback to invoke to check if wasm code generation should be
  // allowed.
  context->GetIsolate()->SetAllowWasmCodeGenerationCallback(
      AllowWasmCodeGenerationCallback);

  // Generate more detailed source positions to code objects. This results in
  // better results when mapping profiling samples to script source.
  v8::CpuProfiler::UseDetailedSourcePositionsForProfiling(
      context->GetIsolate());

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
  node::LoadEnvironment(env);
  gin_helper::EmitEvent(env->isolate(), env->process_object(), "loaded");
}

void NodeBindings::PrepareMessageLoop() {
  // Add dummy handle for libuv, otherwise libuv would quit when there is
  // nothing to do.
  uv_async_init(uv_loop_, &dummy_uv_handle_, nullptr);

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

  // Perform microtask checkpoint after running JavaScript.
  v8::MicrotasksScope script_scope(env->isolate(),
                                   v8::MicrotasksScope::kRunMicrotasks);

  if (browser_env_ != BrowserEnvironment::BROWSER)
    TRACE_EVENT_BEGIN0("devtools.timeline", "FunctionCall");

  // Deal with uv events.
  int r = uv_run(uv_loop_, UV_RUN_NOWAIT);

  if (browser_env_ != BrowserEnvironment::BROWSER)
    TRACE_EVENT_END0("devtools.timeline", "FunctionCall");

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
  uv_async_send(&dummy_uv_handle_);
}

// static
void NodeBindings::EmbedThreadRunner(void* arg) {
  NodeBindings* self = static_cast<NodeBindings*>(arg);

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
