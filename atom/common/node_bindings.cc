// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/node_bindings.h"

#include <string>
#include <vector>

#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/api/locker.h"
#include "atom/common/atom_command_line.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_paths.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

// Force all builtin modules to be referenced so they can actually run their
// DSO constructors, see http://git.io/DRIqCg.
#define REFERENCE_MODULE(name) \
  extern "C" void _register_ ## name(void); \
  void (*fp_register_ ## name)(void) = _register_ ## name
// Electron's builtin modules.
REFERENCE_MODULE(atom_browser_app);
REFERENCE_MODULE(atom_browser_auto_updater);
REFERENCE_MODULE(atom_browser_browser_view);
REFERENCE_MODULE(atom_browser_content_tracing);
REFERENCE_MODULE(atom_browser_debugger);
REFERENCE_MODULE(atom_browser_desktop_capturer);
REFERENCE_MODULE(atom_browser_dialog);
REFERENCE_MODULE(atom_browser_download_item);
REFERENCE_MODULE(atom_browser_global_shortcut);
REFERENCE_MODULE(atom_browser_menu);
REFERENCE_MODULE(atom_browser_net);
REFERENCE_MODULE(atom_browser_power_monitor);
REFERENCE_MODULE(atom_browser_power_save_blocker);
REFERENCE_MODULE(atom_browser_protocol);
REFERENCE_MODULE(atom_browser_render_process_preferences);
REFERENCE_MODULE(atom_browser_session);
REFERENCE_MODULE(atom_browser_system_preferences);
REFERENCE_MODULE(atom_browser_tray);
REFERENCE_MODULE(atom_browser_web_contents);
REFERENCE_MODULE(atom_browser_web_view_manager);
REFERENCE_MODULE(atom_browser_window);
REFERENCE_MODULE(atom_common_asar);
REFERENCE_MODULE(atom_common_clipboard);
REFERENCE_MODULE(atom_common_crash_reporter);
REFERENCE_MODULE(atom_common_native_image);
REFERENCE_MODULE(atom_common_notification);
REFERENCE_MODULE(atom_common_screen);
REFERENCE_MODULE(atom_common_shell);
REFERENCE_MODULE(atom_common_v8_util);
REFERENCE_MODULE(atom_renderer_ipc);
REFERENCE_MODULE(atom_renderer_web_frame);
#undef REFERENCE_MODULE

namespace atom {

namespace {

// Convert the given vector to an array of C-strings. The strings in the
// returned vector are only guaranteed valid so long as the vector of strings
// is not modified.
std::unique_ptr<const char*[]> StringVectorToArgArray(
    const std::vector<std::string>& vector) {
  std::unique_ptr<const char*[]> array(new const char*[vector.size()]);
  for (size_t i = 0; i < vector.size(); ++i) {
    array[i] = vector[i].c_str();
  }
  return array;
}

base::FilePath GetResourcesPath(bool is_browser) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath exec_path(command_line->GetProgram());
  PathService::Get(base::FILE_EXE, &exec_path);

  base::FilePath resources_path =
#if defined(OS_MACOSX)
      is_browser ? exec_path.DirName().DirName().Append("Resources") :
                   exec_path.DirName().DirName().DirName().DirName().DirName()
                            .Append("Resources");
#else
      exec_path.DirName().Append(FILE_PATH_LITERAL("resources"));
#endif
  return resources_path;
}

}  // namespace

NodeBindings::NodeBindings(BrowserEnvironment browser_env)
    : browser_env_(browser_env),
      uv_loop_(browser_env == WORKER ? uv_loop_new() : uv_default_loop()),
      embed_closed_(false),
      uv_env_(nullptr),
      weak_factory_(this) {
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

  // Destroy loop.
  if (uv_loop_ != uv_default_loop())
    uv_loop_delete(uv_loop_);
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

  // Init node.
  // (we assume node::Init would not modify the parameters under embedded mode).
  node::Init(nullptr, nullptr, nullptr, nullptr);

#if defined(OS_WIN)
  // uv_init overrides error mode to suppress the default crash dialog, bring
  // it back if user wants to show it.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (browser_env_ == BROWSER || env->HasVar("ELECTRON_DEFAULT_ERROR_MODE"))
    SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);
#endif
}

node::Environment* NodeBindings::CreateEnvironment(
    v8::Handle<v8::Context> context) {
  auto args = AtomCommandLine::argv();

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
  std::string script_path_str = script_path.AsUTF8Unsafe();
  args.insert(args.begin() + 1, script_path_str.c_str());

  std::unique_ptr<const char*[]> c_argv = StringVectorToArgArray(args);
  node::Environment* env = node::CreateEnvironment(
      new node::IsolateData(context->GetIsolate(), uv_loop_), context,
      args.size(), c_argv.get(), 0, nullptr);

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
  PathService::Get(content::CHILD_PROCESS_EXE, &helper_exec_path);
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
  task_runner_->PostTask(FROM_HERE, base::Bind(&NodeBindings::UvRunOnce,
                                               weak_factory_.GetWeakPtr()));
}

void NodeBindings::WakeupEmbedThread() {
  uv_async_send(&dummy_uv_handle_);
}

// static
void NodeBindings::EmbedThreadRunner(void *arg) {
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
