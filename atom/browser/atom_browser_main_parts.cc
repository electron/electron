// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/api/trackable_object.h"
#include "atom/browser/atom_access_token_store.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_web_ui_controller_factory.h"
#include "atom/browser/bridge_task_runner.h"
#include "atom/browser/browser.h"
#include "atom/browser/javascript_environment.h"
#include "atom/browser/node_debugger.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/asar/asar_util.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "base/command_line.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/child_process_security_policy.h"
#include "device/geolocation/geolocation_delegate.h"
#include "device/geolocation/geolocation_provider.h"
#include "ui/base/l10n/l10n_util.h"
#include "v8/include/v8-debug.h"

#if defined(USE_X11)
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "ui/events/devices/x11/touch_factory_x11.h"
#endif

namespace atom {

namespace {

// A provider of Geolocation services to override AccessTokenStore.
class AtomGeolocationDelegate : public device::GeolocationDelegate {
 public:
  AtomGeolocationDelegate() {
    device::GeolocationProvider::GetInstance()
        ->UserDidOptIntoLocationServices();
  }

  scoped_refptr<device::AccessTokenStore> CreateAccessTokenStore() final {
    return new AtomAccessTokenStore();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomGeolocationDelegate);
};

template<typename T>
void Erase(T* container, typename T::iterator iter) {
  container->erase(iter);
}

}  // namespace

// static
AtomBrowserMainParts* AtomBrowserMainParts::self_ = nullptr;

AtomBrowserMainParts::AtomBrowserMainParts()
    : fake_browser_process_(new BrowserProcess),
      exit_code_(nullptr),
      browser_(new Browser),
      node_bindings_(NodeBindings::Create(NodeBindings::BROWSER)),
      atom_bindings_(new AtomBindings(uv_default_loop())),
      gc_timer_(true, true) {
  DCHECK(!self_) << "Cannot have two AtomBrowserMainParts";
  self_ = this;
  // Register extension scheme as web safe scheme.
  content::ChildProcessSecurityPolicy::GetInstance()->
      RegisterWebSafeScheme("chrome-extension");
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
  asar::ClearArchives();
  // Leak the JavascriptEnvironment on exit.
  // This is to work around the bug that V8 would be waiting for background
  // tasks to finish on exit, while somehow it waits forever in Electron, more
  // about this can be found at https://github.com/electron/electron/issues/4767.
  // On the other handle there is actually no need to gracefully shutdown V8
  // on exit in the main process, we already ensured all necessary resources get
  // cleaned up, and it would make quitting faster.
  ignore_result(js_env_.release());
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

int AtomBrowserMainParts::GetExitCode() {
  return exit_code_ != nullptr ? *exit_code_ : 0;
}

base::Closure AtomBrowserMainParts::RegisterDestructionCallback(
    const base::Closure& callback) {
  auto iter = destructors_.insert(destructors_.end(), callback);
  return base::Bind(&Erase<std::list<base::Closure>>, &destructors_, iter);
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

  // Create the global environment.
  node::Environment* env =
      node_bindings_->CreateEnvironment(js_env_->context());
  node_env_.reset(new NodeEnvironment(env));

  // Enable support for v8 inspector
  node_debugger_.reset(new NodeDebugger(env));
  node_debugger_->Start(js_env_->platform());

  // Add Electron extended APIs.
  atom_bindings_->BindTo(js_env_->isolate(), env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(env);

  // Wrap the uv loop with global env.
  node_bindings_->set_uv_env(env);
}

int AtomBrowserMainParts::PreCreateThreads() {
  fake_browser_process_->SetApplicationLocale(
      l10n_util::GetApplicationLocale(""));
  return brightray::BrowserMainParts::PreCreateThreads();
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  js_env_->OnMessageLoopCreated();

  // Run user's main script before most things get initialized, so we can have
  // a chance to setup everything.
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

#if defined(USE_X11)
  ui::TouchFactory::SetTouchDeviceListFromCommandLine();
#endif

  // Start idle gc.
  gc_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMinutes(1),
      base::Bind(&v8::Isolate::LowMemoryNotification,
                 base::Unretained(js_env_->isolate())));

  content::WebUIControllerFactory::RegisterFactory(
      AtomWebUIControllerFactory::GetInstance());

  brightray::BrowserMainParts::PreMainMessageLoopRun();
  bridge_task_runner_->MessageLoopIsReady();
  bridge_task_runner_ = nullptr;

#if defined(USE_X11)
  libgtkui::GtkInitFromCommandLine(*base::CommandLine::ForCurrentProcess());
#endif

#if !defined(OS_MACOSX)
  // The corresponding call in macOS is in AtomApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  std::unique_ptr<base::DictionaryValue> empty_info(new base::DictionaryValue);
  Browser::Get()->DidFinishLaunching(*empty_info);
#endif

  // Notify observers that main thread message loop was initialized.
  Browser::Get()->PreMainMessageLoopRun();
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
  device::GeolocationProvider::SetGeolocationDelegate(
      new AtomGeolocationDelegate());
}

void AtomBrowserMainParts::PostMainMessageLoopRun() {
  brightray::BrowserMainParts::PostMainMessageLoopRun();

  js_env_->OnMessageLoopDestroying();

#if defined(OS_MACOSX)
  FreeAppDelegate();
#endif

  // Make sure destruction callbacks are called before message loop is
  // destroyed, otherwise some objects that need to be deleted on IO thread
  // won't be freed.
  // We don't use ranged for loop because iterators are getting invalided when
  // the callback runs.
  for (auto iter = destructors_.begin(); iter != destructors_.end();) {
    base::Closure& callback = *iter;
    ++iter;
    callback.Run();
  }
}

}  // namespace atom
