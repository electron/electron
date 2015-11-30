// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/bridge_task_runner.h"
#include "atom/browser/browser.h"
#include "atom/browser/javascript_environment.h"
#include "atom/browser/node_debugger.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "base/command_line.h"
#include "base/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "v8/include/v8-debug.h"

#if defined(USE_X11)
#include "chrome/browser/ui/libgtk2ui/gtk2_util.h"
#endif

namespace atom {

// static
AtomBrowserMainParts* AtomBrowserMainParts::self_ = NULL;

AtomBrowserMainParts::AtomBrowserMainParts()
    : fake_browser_process_(new BrowserProcess),
      exit_code_(nullptr),
      browser_(new Browser),
      node_bindings_(NodeBindings::Create(true)),
      atom_bindings_(new AtomBindings),
      gc_timer_(true, true) {
  DCHECK(!self_) << "Cannot have two AtomBrowserMainParts";
  self_ = this;
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
}

// static
AtomBrowserMainParts* AtomBrowserMainParts::Get() {
  DCHECK(self_);
  return self_;
}

bool AtomBrowserMainParts::SetExitCode(int code) {
  if (!exit_code_)
    return false;

  *exit_code_ = code;
  return true;
}

void AtomBrowserMainParts::RegisterDestructionCallback(
    const base::Closure& callback) {
  destruction_callbacks_.push_back(callback);
}

void AtomBrowserMainParts::PreEarlyInitialization() {
  brightray::BrowserMainParts::PreEarlyInitialization();
#if defined(OS_POSIX)
  HandleSIGCHLD();
#endif
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  brightray::BrowserMainParts::PostEarlyInitialization();

  // Temporary set the bridge_task_runner_ as current thread's task runner,
  // so we can fool gin::PerIsolateData to use it as its task runner, instead
  // of getting current message loop's task runner, which is null for now.
  bridge_task_runner_ = new BridgeTaskRunner;
  base::ThreadTaskRunnerHandle handle(bridge_task_runner_);

  // The ProxyResolverV8 has setup a complete V8 environment, in order to
  // avoid conflicts we only initialize our V8 environment after that.
  js_env_.reset(new JavascriptEnvironment);

  node_bindings_->Initialize();

  // Support the "--debug" switch.
  node_debugger_.reset(new NodeDebugger(js_env_->isolate()));

  // Create the global environment.
  global_env = node_bindings_->CreateEnvironment(js_env_->context());

  // Make sure node can get correct environment when debugging.
  if (node_debugger_->IsRunning())
    global_env->AssignToContext(v8::Debug::GetDebugContext());

  // Add atom-shell extended APIs.
  atom_bindings_->BindTo(js_env_->isolate(), global_env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(global_env);
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  // Run user's main script before most things get initialized, so we can have
  // a chance to setup everything.
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

  // Start idle gc.
  gc_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMinutes(1),
      base::Bind(base::IgnoreResult(&v8::Isolate::IdleNotification),
                 base::Unretained(js_env_->isolate()),
                 1000));

  brightray::BrowserMainParts::PreMainMessageLoopRun();
  BridgeTaskRunner::MessageLoopIsReady();

#if defined(USE_X11)
  libgtk2ui::GtkInitFromCommandLine(*base::CommandLine::ForCurrentProcess());
#endif

#if !defined(OS_MACOSX)
  // The corresponding call in OS X is in AtomApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  Browser::Get()->DidFinishLaunching();
#endif
}

bool AtomBrowserMainParts::MainMessageLoopRun(int* result_code) {
  exit_code_ = result_code;
  return brightray::BrowserMainParts::MainMessageLoopRun(result_code);
}

void AtomBrowserMainParts::PostMainMessageLoopStart() {
  brightray::BrowserMainParts::PostMainMessageLoopStart();
#if defined(OS_POSIX)
  HandleShutdownSignals();
#endif
}

void AtomBrowserMainParts::PostMainMessageLoopRun() {
  brightray::BrowserMainParts::PostMainMessageLoopRun();

#if defined(OS_MACOSX)
  FreeAppDelegate();
#endif

  // Make sure destruction callbacks are called before message loop is
  // destroyed, otherwise some objects that need to be deleted on IO thread
  // won't be freed.
  for (const auto& callback : destruction_callbacks_)
    callback.Run();

  // Destroy JavaScript environment immediately after running destruction
  // callbacks.
  gc_timer_.Stop();
  node_debugger_.reset();
  atom_bindings_.reset();
  node_bindings_.reset();
  js_env_.reset();
}

}  // namespace atom
