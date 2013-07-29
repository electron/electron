// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/node_bindings.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"
#include "vendor/node/src/node_javascript.h"

#if defined(OS_WIN)
#include "base/strings/utf_string_conversions.h"
#endif

using content::BrowserThread;

namespace atom {

namespace {

void UvNoOp(uv_async_t* handle, int status) {
}

}  // namespace

NodeBindings::NodeBindings(bool is_browser)
    : is_browser_(is_browser),
      message_loop_(NULL),
      uv_loop_(uv_default_loop()),
      embed_closed_(false) {
}

NodeBindings::~NodeBindings() {
  // Clear uv.
  embed_closed_ = true;
  WakeupEmbedThread();
  uv_thread_join(&embed_thread_);
  uv_sem_destroy(&embed_sem_);
  uv_timer_stop(&idle_timer_);
}

void NodeBindings::Initialize() {
  CommandLine::StringVector str_argv = CommandLine::ForCurrentProcess()->argv();

#if defined(OS_WIN)
  std::vector<std::string> utf8_str_argv;
  utf8_str_argv.reserve(str_argv.size());
#endif

  // Convert string vector to char* array.
  std::vector<char*> argv(str_argv.size(), NULL);
  for (size_t i = 0; i < str_argv.size(); ++i) {
#if defined(OS_WIN)
    utf8_str_argv.push_back(UTF16ToUTF8(str_argv[i]));
    argv[i] = const_cast<char*>(utf8_str_argv[i].c_str());
#else
    argv[i] = const_cast<char*>(str_argv[i].c_str());
#endif
  }

  // Init idle GC.
  uv_timer_init(uv_default_loop(), &idle_timer_);
  uv_timer_start(&idle_timer_, IdleCallback, 5000, 5000);

  // Open node's error reporting system for browser process.
  node::g_standalone_mode = is_browser_;

  // Init node.
  node::Init(argv.size(), &argv[0]);
  v8::V8::Initialize();

  // Load node.js.
  v8::HandleScope scope;
  node::g_context = v8::Persistent<v8::Context>::New(
      node::node_isolate, v8::Context::New(node::node_isolate));
  {
    v8::Context::Scope context_scope(node::g_context);
    v8::Handle<v8::Object> process = node::SetupProcessObject(
        argv.size(), &argv[0]);

    // Tell node.js we are in browser or renderer.
    v8::Handle<v8::String> type =
        is_browser_ ? v8::String::New("browser") : v8::String::New("renderer");
    process->Set(v8::String::New("__atom_type"), type);
  }
}

void NodeBindings::Load() {
  v8::HandleScope scope;
  v8::Context::Scope context_scope(node::g_context);
  node::Load(node::process);
}

void NodeBindings::BindTo(WebKit::WebFrame* frame) {
  v8::HandleScope handle_scope;

  v8::Handle<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope scope(context);

  // Erase security token.
  context->SetSecurityToken(node::g_context->GetSecurityToken());

  // Evaluate cefode.js.
  v8::Handle<v8::Script> script = node::CompileCefodeMainSource();
  v8::Local<v8::Value> result = script->Run();

  // Run the script of cefode.js.
  std::string script_path(GURL(frame->document().url()).path());
  v8::Handle<v8::Value> args[2] = {
    v8::Local<v8::Value>::New(node::process),
    v8::String::New(script_path.c_str(), script_path.size())
  };
  v8::Local<v8::Function>::Cast(result)->Call(context->Global(), 2, args);
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

  // Enter node context while dealing with uv events.
  v8::HandleScope scope;
  v8::Context::Scope context_scope(node::g_context);

  // Deal with uv events.
  int r = uv_run(uv_loop_, (uv_run_mode)(UV_RUN_ONCE | UV_RUN_NOWAIT));
  if (r == 0 || uv_loop_->stop_flag != 0)
    message_loop_->QuitWhenIdle();  // Quit from uv.

  // Tell the worker thread to continue polling.
  uv_sem_post(&embed_sem_);
}

void NodeBindings::WakeupMainThread() {
  DCHECK(message_loop_);
  message_loop_->PostTask(FROM_HERE, base::Bind(&NodeBindings::UvRunOnce,
                                                base::Unretained(this)));
}

void NodeBindings::WakeupEmbedThread() {
  uv_async_send(&dummy_uv_handle_);
}

// static
void NodeBindings::EmbedThreadRunner(void *arg) {
  NodeBindings* self = static_cast<NodeBindings*>(arg);

  while (!self->embed_closed_) {
    // Wait for the main loop to deal with events.
    uv_sem_wait(&self->embed_sem_);

    self->PollEvents();

    // Deal with event in main thread.
    self->WakeupMainThread();
  }
}

// static
void NodeBindings::IdleCallback(uv_timer_t*, int) {
  v8::V8::IdleNotification();
}

}  // namespace atom
