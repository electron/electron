// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include <utility>

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/api/trackable_object.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_web_ui_controller_factory.h"
#include "atom/browser/browser.h"
#include "atom/browser/io_thread.h"
#include "atom/browser/javascript_environment.h"
#include "atom/browser/media/media_capture_devices_dispatcher.h"
#include "atom/browser/node_debugger.h"
#include "atom/browser/ui/devtools_manager_delegate.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/asar/asar_util.h"
#include "atom/common/node_bindings.h"
#include "base/command_line.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/icon_manager.h"
#include "chrome/browser/net/chrome_net_log_helper.h"
#include "components/net_log/chrome_net_log.h"
#include "components/net_log/net_export_file_writer.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "content/public/common/service_manager_connection.h"
#include "electron/buildflags/buildflags.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/idle/idle.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(USE_X11)
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "ui/events/devices/x11/touch_factory_x11.h"
#endif

#if defined(OS_MACOSX)
#include "atom/browser/ui/cocoa/views_delegate_mac.h"
#else
#include "atom/browser/ui/views/atom_views_delegate.h"
#endif

// Must be included after all other headers.
#include "atom/common/node_includes.h"

namespace atom {

namespace {

template <typename T>
void Erase(T* container, typename T::iterator iter) {
  container->erase(iter);
}

}  // namespace

// static
AtomBrowserMainParts* AtomBrowserMainParts::self_ = nullptr;

AtomBrowserMainParts::AtomBrowserMainParts(
    const content::MainFunctionParams& params)
    : fake_browser_process_(new BrowserProcessImpl),
      browser_(new Browser),
      node_bindings_(NodeBindings::Create(NodeBindings::BROWSER)),
      atom_bindings_(new AtomBindings(uv_default_loop())),
      main_function_params_(params) {
  DCHECK(!self_) << "Cannot have two AtomBrowserMainParts";
  self_ = this;
  // Register extension scheme as web safe scheme.
  content::ChildProcessSecurityPolicy::GetInstance()->RegisterWebSafeScheme(
      "chrome-extension");
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
  asar::ClearArchives();
  // Leak the JavascriptEnvironment on exit.
  // This is to work around the bug that V8 would be waiting for background
  // tasks to finish on exit, while somehow it waits forever in Electron, more
  // about this can be found at
  // https://github.com/electron/electron/issues/4767. On the other handle there
  // is actually no need to gracefully shutdown V8 on exit in the main process,
  // we already ensured all necessary resources get cleaned up, and it would
  // make quitting faster.
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

void AtomBrowserMainParts::RegisterDestructionCallback(
    base::OnceClosure callback) {
  // The destructors should be called in reversed order, so dependencies between
  // JavaScript objects can be correctly resolved.
  // For example WebContentsView => WebContents => Session.
  destructors_.insert(destructors_.begin(), std::move(callback));
}

int AtomBrowserMainParts::PreEarlyInitialization() {
  const int result = brightray::BrowserMainParts::PreEarlyInitialization();
  if (result != service_manager::RESULT_CODE_NORMAL_EXIT)
    return result;

#if defined(OS_POSIX)
  HandleSIGCHLD();
#endif

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  brightray::BrowserMainParts::PostEarlyInitialization();

  // A workaround was previously needed because there was no ThreadTaskRunner
  // set.  If this check is failing we may need to re-add that workaround
  DCHECK(base::ThreadTaskRunnerHandle::IsSet());

  // The ProxyResolverV8 has setup a complete V8 environment, in order to
  // avoid conflicts we only initialize our V8 environment after that.
  js_env_.reset(new JavascriptEnvironment(node_bindings_->uv_loop()));

  node_bindings_->Initialize();
  // Create the global environment.
  node::Environment* env = node_bindings_->CreateEnvironment(
      js_env_->context(), js_env_->platform());
  node_env_.reset(new NodeEnvironment(env));

  // Enable support for v8 inspector
  node_debugger_.reset(new NodeDebugger(env));
  node_debugger_->Start();

  // Add Electron extended APIs.
  atom_bindings_->BindTo(js_env_->isolate(), env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(env);

  // Wrap the uv loop with global env.
  node_bindings_->set_uv_env(env);

  // We already initialized the feature list in
  // brightray::BrowserMainParts::PreEarlyInitialization(), but
  // the user JS script would not have had a chance to alter the command-line
  // switches at that point. Lets reinitialize it here to pick up the
  // command-line changes.
  base::FeatureList::ClearInstanceForTesting();
  brightray::BrowserMainParts::InitializeFeatureList();
}

int AtomBrowserMainParts::PreCreateThreads() {
  const int result = brightray::BrowserMainParts::PreCreateThreads();
  if (!result) {
    fake_browser_process_->SetApplicationLocale(
        brightray::BrowserClient::Get()->GetApplicationLocale());
  }

  // Force MediaCaptureDevicesDispatcher to be created on UI thread.
  MediaCaptureDevicesDispatcher::GetInstance();

#if defined(OS_MACOSX)
  ui::InitIdleMonitor();
#endif

  net_log_ = std::make_unique<net_log::ChromeNetLog>();
  auto& command_line = main_function_params_.command_line;
  // start net log trace if --log-net-log is passed in the command line.
  if (command_line.HasSwitch(network::switches::kLogNetLog)) {
    base::FilePath log_file =
        command_line.GetSwitchValuePath(network::switches::kLogNetLog);
    if (!log_file.empty()) {
      net_log_->StartWritingToFile(
          log_file, GetNetCaptureModeFromCommandLine(command_line),
          command_line.GetCommandLineString(), std::string());
    }
  }
  // Initialize net log file exporter.
  net_log_->net_export_file_writer()->Initialize();

  // Manage global state of net and other IO thread related.
  io_thread_ = std::make_unique<IOThread>(net_log_.get());

  return result;
}

void AtomBrowserMainParts::PostDestroyThreads() {
  brightray::BrowserMainParts::PostDestroyThreads();
  io_thread_.reset();
}

void AtomBrowserMainParts::ToolkitInitialized() {
  brightray::BrowserMainParts::ToolkitInitialized();
#if defined(OS_MACOSX)
  views_delegate_.reset(new ViewsDelegateMac);
#else
  views_delegate_.reset(new ViewsDelegate);
#endif
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  // Run user's main script before most things get initialized, so we can have
  // a chance to setup everything.
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

#if defined(USE_X11)
  ui::TouchFactory::SetTouchDeviceListFromCommandLine();
#endif

  // Start idle gc.
  gc_timer_.Start(FROM_HERE, base::TimeDelta::FromMinutes(1),
                  base::Bind(&v8::Isolate::LowMemoryNotification,
                             base::Unretained(js_env_->isolate())));

  content::WebUIControllerFactory::RegisterFactory(
      AtomWebUIControllerFactory::GetInstance());

  // --remote-debugging-port
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kRemoteDebuggingPort))
    DevToolsManagerDelegate::StartHttpHandler();

#if defined(USE_X11)
  libgtkui::GtkInitFromCommandLine(*base::CommandLine::ForCurrentProcess());
#endif

#if !defined(OS_MACOSX)
  // The corresponding call in macOS is in AtomApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  Browser::Get()->DidFinishLaunching(base::DictionaryValue());
#endif

  // Notify observers that main thread message loop was initialized.
  Browser::Get()->PreMainMessageLoopRun();
}

bool AtomBrowserMainParts::MainMessageLoopRun(int* result_code) {
  js_env_->OnMessageLoopCreated();
  exit_code_ = result_code;
  return brightray::BrowserMainParts::MainMessageLoopRun(result_code);
}

void AtomBrowserMainParts::PreDefaultMainMessageLoopRun(
    base::OnceClosure quit_closure) {
  Browser::SetMainMessageLoopQuitClosure(std::move(quit_closure));
}

void AtomBrowserMainParts::PostMainMessageLoopStart() {
  brightray::BrowserMainParts::PostMainMessageLoopStart();
#if defined(OS_POSIX)
  HandleShutdownSignals();
#endif
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
    base::OnceClosure callback = std::move(*iter);
    if (!callback.is_null())
      std::move(callback).Run();
    ++iter;
  }
}

device::mojom::GeolocationControl*
AtomBrowserMainParts::GetGeolocationControl() {
  if (geolocation_control_)
    return geolocation_control_.get();

  auto request = mojo::MakeRequest(&geolocation_control_);
  if (!content::ServiceManagerConnection::GetForProcess())
    return geolocation_control_.get();

  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(device::mojom::kServiceName, std::move(request));
  return geolocation_control_.get();
}

IconManager* AtomBrowserMainParts::GetIconManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!icon_manager_.get())
    icon_manager_.reset(new IconManager);
  return icon_manager_.get();
}

}  // namespace atom
