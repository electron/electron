// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/node_bindings.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/api/locker.h"
#include "atom/common/atom_command_line.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_paths.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

#define ELECTRON_BUILTIN_MODULES(V)          \
  V(atom_browser_app)                        \
  V(atom_browser_auto_updater)               \
  V(atom_browser_browser_view)               \
  V(atom_browser_content_tracing)            \
  V(atom_browser_debugger)                   \
  V(atom_browser_dialog)                     \
  V(atom_browser_download_item)              \
  V(atom_browser_global_shortcut)            \
  V(atom_browser_in_app_purchase)            \
  V(atom_browser_menu)                       \
  V(atom_browser_net)                        \
  V(atom_browser_net_log)                    \
  V(atom_browser_power_monitor)              \
  V(atom_browser_power_save_blocker)         \
  V(atom_browser_protocol)                   \
  V(atom_browser_render_process_preferences) \
  V(atom_browser_session)                    \
  V(atom_browser_system_preferences)         \
  V(atom_browser_top_level_window)           \
  V(atom_browser_tray)                       \
  V(atom_browser_web_contents)               \
  V(atom_browser_web_contents_view)          \
  V(atom_browser_view)                       \
  V(atom_browser_web_view_manager)           \
  V(atom_browser_window)                     \
  V(atom_common_asar)                        \
  V(atom_common_clipboard)                   \
  V(atom_common_crash_reporter)              \
  V(atom_common_features)                    \
  V(atom_common_native_image)                \
  V(atom_common_notification)                \
  V(atom_common_screen)                      \
  V(atom_common_shell)                       \
  V(atom_common_v8_util)                     \
  V(atom_renderer_ipc)                       \
  V(atom_renderer_web_frame)

#define ELECTRON_VIEW_MODULES(V) \
  V(atom_browser_box_layout)     \
  V(atom_browser_button)         \
  V(atom_browser_label_button)   \
  V(atom_browser_layout_manager) \
  V(atom_browser_text_field)

#define ELECTRON_DESKTOP_CAPTURER_MODULE(V) V(atom_browser_desktop_capturer)

// This is used to load built-in modules. Instead of using
// __attribute__((constructor)), we call the _register_<modname>
// function for each built-in modules explicitly. This is only
// forward declaration. The definitions are in each module's
// implementation when calling the NODE_BUILTIN_MODULE_CONTEXT_AWARE.
#define V(modname) void _register_##modname();
ELECTRON_BUILTIN_MODULES(V)
#if defined(ENABLE_VIEW_API)
ELECTRON_VIEW_MODULES(V)
#endif
#if defined(ENABLE_DESKTOP_CAPTURER)
ELECTRON_DESKTOP_CAPTURER_MODULE(V)
#endif
#undef V

namespace {

void stop_and_close_uv_loop(uv_loop_t* loop) {
  // Close any active handles
  uv_stop(loop);
  uv_walk(loop,
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

}  // namespace

namespace atom {

namespace {

// Convert the given vector to an array of C-strings. The strings in the
// returned vector are only guaranteed valid so long as the vector of strings
// is not modified.
std::unique_ptr<const char* []> StringVectorToArgArray(
    const std::vector<std::string>& vector) {
  std::unique_ptr<const char* []> array(new const char*[vector.size()]);
  for (size_t i = 0; i < vector.size(); ++i) {
    array[i] = vector[i].c_str();
  }
  return array;
}

base::FilePath GetResourcesPath(bool is_browser) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath exec_path(command_line->GetProgram());
  base::PathService::Get(base::FILE_EXE, &exec_path);

  base::FilePath resources_path =
#if defined(OS_MACOSX)
      is_browser
          ? exec_path.DirName().DirName().Append("Resources")
          : exec_path.DirName().DirName().DirName().DirName().DirName().Append(
                "Resources");
#else
      exec_path.DirName().Append(FILE_PATH_LITERAL("resources"));
#endif
  return resources_path;
}

}  // namespace

NodeBindings::NodeBindings(BrowserEnvironment browser_env)
    : browser_env_(browser_env), weak_factory_(this) {
  if (browser_env == WORKER) {
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
#if defined(ENABLE_VIEW_API)
  ELECTRON_VIEW_MODULES(V)
#endif
#if defined(ENABLE_DESKTOP_CAPTURER)
  ELECTRON_DESKTOP_CAPTURER_MODULE(V)
#endif
#undef V
}

bool NodeBindings::IsInitialized() {
  return g_is_initialized;
}

base::FilePath::StringType NodeBindings::GetHelperResourcesPath() {
  return GetResourcesPath(false).value();
}

void NodeBindings::Initialize() {
  // Open node's error reporting system for browser process.
  node::g_standalone_mode = browser_env_ == BROWSER;
  node::g_upstream_node_mode = false;

#if defined(OS_LINUX)
  // Get real command line in renderer process forked by zygote.
  if (browser_env_ != BROWSER)
    AtomCommandLine::InitializeFromCommandLine();
#endif

  // Explicitly register electron's builtin modules.
  RegisterBuiltinModules();

  // Init node.
  // (we assume node::Init would not modify the parameters under embedded mode).
  // NOTE: If you change this line, please ping @codebytere or @MarshallOfSound
  node::Init(nullptr, nullptr, nullptr, nullptr);

#if defined(OS_WIN)
  // uv_init overrides error mode to suppress the default crash dialog, bring
  // it back if user wants to show it.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (browser_env_ == BROWSER || env->HasVar("ELECTRON_DEFAULT_ERROR_MODE"))
    SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);
#endif

  g_is_initialized = true;
}

node::Environment* NodeBindings::CreateEnvironment(
    v8::Handle<v8::Context> context,
    node::MultiIsolatePlatform* platform) {
#if defined(OS_WIN)
  auto& atom_args = AtomCommandLine::argv();
  std::vector<std::string> args(atom_args.size());
  std::transform(atom_args.cbegin(), atom_args.cend(), args.begin(),
                 [](auto& a) { return base::WideToUTF8(a); });
#else
  auto args = AtomCommandLine::argv();
#endif

  // Feed node the path to initialization script.
  base::FilePath::StringType process_type;
  switch (browser_env_) {
    case BROWSER:
      process_type = FILE_PATH_LITERAL("browser");
      break;
    case RENDERER:
      process_type = FILE_PATH_LITERAL("renderer");
      break;
    case WORKER:
      process_type = FILE_PATH_LITERAL("worker");
      break;
  }
  base::FilePath resources_path = GetResourcesPath(browser_env_ == BROWSER);
  base::FilePath script_path =
      resources_path.Append(FILE_PATH_LITERAL("electron.asar"))
          .Append(process_type)
          .Append(FILE_PATH_LITERAL("init.js"));
  args.insert(args.begin() + 1, script_path.AsUTF8Unsafe());

  std::unique_ptr<const char* []> c_argv = StringVectorToArgArray(args);
  node::Environment* env = node::CreateEnvironment(
      node::CreateIsolateData(context->GetIsolate(), uv_loop_, platform),
      context, args.size(), c_argv.get(), 0, nullptr);

  if (browser_env_ == BROWSER) {
    // SetAutorunMicrotasks is no longer called in node::CreateEnvironment
    // so instead call it here to match expected node behavior
    context->GetIsolate()->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
  } else {
    // Node uses the deprecated SetAutorunMicrotasks(false) mode, we should
    // switch to use the scoped policy to match blink's behavior.
    context->GetIsolate()->SetMicrotasksPolicy(v8::MicrotasksPolicy::kScoped);
  }

  mate::Dictionary process(context->GetIsolate(), env->process_object());
  process.Set("type", process_type);
  process.Set("resourcesPath", resources_path);
  // Do not set DOM globals for renderer process.
  if (browser_env_ != BROWSER)
    process.Set("_noBrowserGlobals", resources_path);
  // The path to helper app.
  base::FilePath helper_exec_path;
  base::PathService::Get(content::CHILD_PROCESS_EXE, &helper_exec_path);
  process.Set("helperExecPath", helper_exec_path);

  return env;
}

void NodeBindings::LoadEnvironment(node::Environment* env) {
  node::LoadEnvironment(env);
  mate::EmitEvent(env->isolate(), env->process_object(), "loaded");
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
  mate::Locker locker(env->isolate());
  v8::HandleScope handle_scope(env->isolate());

  // Enter node context while dealing with uv events.
  v8::Context::Scope context_scope(env->context());

  // Perform microtask checkpoint after running JavaScript.
  v8::MicrotasksScope script_scope(env->isolate(),
                                   v8::MicrotasksScope::kRunMicrotasks);

  if (browser_env_ != BROWSER)
    TRACE_EVENT_BEGIN0("devtools.timeline", "FunctionCall");

  // Deal with uv events.
  int r = uv_run(uv_loop_, UV_RUN_NOWAIT);

  if (browser_env_ != BROWSER)
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

}  // namespace atom
