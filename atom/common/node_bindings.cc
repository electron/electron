// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/node_bindings.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/locker.h"

#if defined(OS_WIN)
#include "base/strings/utf_string_conversions.h"
#endif

#include "atom/common/node_includes.h"

using content::BrowserThread;

// Forward declaration of internal node functions.
namespace node {
void Init(int*, const char**, int*, const char***);
}

// Force all builtin modules to be referenced so they can actually run their
// DSO constructors, see http://git.io/DRIqCg.
#define REFERENCE_MODULE(name) \
  extern "C" void _register_ ## name(void); \
  void (*fp_register_ ## name)(void) = _register_ ## name
// Node's builtin modules.
REFERENCE_MODULE(cares_wrap);
REFERENCE_MODULE(fs_event_wrap);
REFERENCE_MODULE(buffer);
REFERENCE_MODULE(contextify);
REFERENCE_MODULE(crypto);
REFERENCE_MODULE(fs);
REFERENCE_MODULE(http_parser);
REFERENCE_MODULE(os);
REFERENCE_MODULE(v8);
REFERENCE_MODULE(zlib);
REFERENCE_MODULE(pipe_wrap);
REFERENCE_MODULE(process_wrap);
REFERENCE_MODULE(signal_wrap);
REFERENCE_MODULE(smalloc);
REFERENCE_MODULE(spawn_sync);
REFERENCE_MODULE(tcp_wrap);
REFERENCE_MODULE(timer_wrap);
REFERENCE_MODULE(tls_wrap);
REFERENCE_MODULE(tty_wrap);
REFERENCE_MODULE(udp_wrap);
REFERENCE_MODULE(uv);
// Atom Shell's builtin modules.
REFERENCE_MODULE(atom_browser_app);
REFERENCE_MODULE(atom_browser_auto_updater);
REFERENCE_MODULE(atom_browser_content_tracing);
REFERENCE_MODULE(atom_browser_dialog);
REFERENCE_MODULE(atom_browser_menu);
REFERENCE_MODULE(atom_browser_power_monitor);
REFERENCE_MODULE(atom_browser_protocol);
REFERENCE_MODULE(atom_browser_global_shortcut);
REFERENCE_MODULE(atom_browser_tray);
REFERENCE_MODULE(atom_browser_web_contents);
REFERENCE_MODULE(atom_browser_window);
REFERENCE_MODULE(atom_common_asar);
REFERENCE_MODULE(atom_common_clipboard);
REFERENCE_MODULE(atom_common_crash_reporter);
REFERENCE_MODULE(atom_common_id_weak_map);
REFERENCE_MODULE(atom_common_screen);
REFERENCE_MODULE(atom_common_shell);
REFERENCE_MODULE(atom_common_v8_util);
REFERENCE_MODULE(atom_renderer_ipc);
REFERENCE_MODULE(atom_renderer_web_frame);
#undef REFERENCE_MODULE

namespace atom {

namespace {

// Empty callback for async handle.
void UvNoOp(uv_async_t* handle) {
}

// Convert the given vector to an array of C-strings. The strings in the
// returned vector are only guaranteed valid so long as the vector of strings
// is not modified.
scoped_ptr<const char*[]> StringVectorToArgArray(
    const std::vector<std::string>& vector) {
  scoped_ptr<const char*[]> array(new const char*[vector.size()]);
  for (size_t i = 0; i < vector.size(); ++i)
    array[i] = vector[i].c_str();
  return array.Pass();
}

#if defined(OS_WIN)
std::vector<std::string> String16VectorToStringVector(
    const std::vector<base::string16>& vector) {
  std::vector<std::string> utf8_vector;
  utf8_vector.reserve(vector.size());
  for (size_t i = 0; i < vector.size(); ++i)
    utf8_vector.push_back(base::UTF16ToUTF8(vector[i]));
  return utf8_vector;
}
#endif

}  // namespace

node::Environment* global_env = nullptr;

NodeBindings::NodeBindings(bool is_browser)
    : is_browser_(is_browser),
      message_loop_(nullptr),
      uv_loop_(uv_default_loop()),
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
}

void NodeBindings::Initialize() {
  // Open node's error reporting system for browser process.
  node::g_standalone_mode = is_browser_;
  node::g_upstream_node_mode = false;

  // Init node.
  // (we assume it would not node::Init would not modify the parameters under
  // embedded mode).
  node::Init(nullptr, nullptr, nullptr, nullptr);
}

node::Environment* NodeBindings::CreateEnvironment(
    v8::Handle<v8::Context> context) {
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  std::vector<std::string> args =
#if defined(OS_WIN)
      String16VectorToStringVector(command_line->argv());
#else
      command_line->argv();
#endif

  // Feed node the path to initialization script.
  base::FilePath exec_path(command_line->argv()[0]);
  PathService::Get(base::FILE_EXE, &exec_path);
  base::FilePath resources_path =
#if defined(OS_MACOSX)
      is_browser_ ? exec_path.DirName().DirName().Append("Resources") :
                    exec_path.DirName().DirName().DirName().DirName().DirName()
                             .Append("Resources");
#else
      exec_path.DirName().Append(FILE_PATH_LITERAL("resources"));
#endif
  base::FilePath script_path =
      resources_path.Append(FILE_PATH_LITERAL("atom"))
                    .Append(is_browser_ ? FILE_PATH_LITERAL("browser") :
                                          FILE_PATH_LITERAL("renderer"))
                    .Append(FILE_PATH_LITERAL("lib"))
                    .Append(FILE_PATH_LITERAL("init.js"));
  std::string script_path_str = script_path.AsUTF8Unsafe();
  args.insert(args.begin() + 1, script_path_str.c_str());

  scoped_ptr<const char*[]> c_argv = StringVectorToArgArray(args);
  return node::CreateEnvironment(context->GetIsolate(),
                                 uv_default_loop(),
                                 context,
                                 args.size(), c_argv.get(),
                                 0, nullptr);
}

void NodeBindings::LoadEnvironment(node::Environment* env) {
  node::LoadEnvironment(env);
}

void NodeBindings::PrepareMessageLoop() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Add dummy handle for libuv, otherwise libuv would quit when there is
  // nothing to do.
  uv_async_init(uv_loop_, &dummy_uv_handle_, UvNoOp);

  // Start worker that will interrupt main loop when having uv events.
  uv_sem_init(&embed_sem_, 0);
  uv_thread_create(&embed_thread_, EmbedThreadRunner, this);
}

void NodeBindings::RunMessageLoop() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  // The MessageLoop should have been created, remember the one in main thread.
  message_loop_ = base::MessageLoop::current();

  // Run uv loop for once to give the uv__io_poll a chance to add all events.
  UvRunOnce();
}

void NodeBindings::UvRunOnce() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  // By default the global env would be used unless user specified another one
  // (this happens for renderer process, which wraps the uv loop with web page
  // context).
  node::Environment* env = uv_env() ? uv_env() : global_env;

  // Use Locker in browser process.
  mate::Locker locker(env->isolate());
  v8::HandleScope handle_scope(env->isolate());

  // Enter node context while dealing with uv events.
  v8::Context::Scope context_scope(env->context());

  // Deal with uv events.
  int r = uv_run(uv_loop_, UV_RUN_NOWAIT);
  if (r == 0 || uv_loop_->stop_flag != 0)
    message_loop_->QuitWhenIdle();  // Quit from uv.

  // Tell the worker thread to continue polling.
  uv_sem_post(&embed_sem_);
}

void NodeBindings::WakeupMainThread() {
  DCHECK(message_loop_);
  message_loop_->PostTask(FROM_HERE, base::Bind(&NodeBindings::UvRunOnce,
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
