// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_browser_main_parts.h"

#include <memory>

#include <utility>

#if defined(OS_LINUX)
#include <glib.h>  // for g_setenv()
#endif

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/icon_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/device_service.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "electron/buildflags/buildflags.h"
#include "media/base/localized_strings.h"
#include "services/network/public/cpp/features.h"
#include "services/tracing/public/cpp/stack_sampling/tracing_sampler_profiler.h"
#include "shell/app/electron_main_delegate.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_paths.h"
#include "shell/browser/electron_web_ui_controller_factory.h"
#include "shell/browser/feature_list.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/media/media_capture_devices_dispatcher.h"
#include "shell/browser/node_debugger.h"
#include "shell/browser/ui/devtools_manager_delegate.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/application_info.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/gin_helper/trackable_object.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "ui/base/idle/idle.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/ui_base_switches.h"

#if defined(USE_AURA)
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/wm/core/wm_state.h"
#endif

#if defined(USE_X11)
#include "base/environment.h"
#include "base/nix/xdg_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/ui/gtk/gtk_ui.h"
#include "chrome/browser/ui/gtk/gtk_util.h"
#include "ui/base/x/x11_util.h"
#include "ui/base/x/x11_util_internal.h"
#include "ui/events/devices/x11/touch_factory_x11.h"
#include "ui/views/linux_ui/linux_ui.h"
#endif

#if defined(OS_WIN)
#include "ui/base/cursor/cursor_loader_win.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/display/win/dpi.h"
#include "ui/gfx/system_fonts_win.h"
#include "ui/strings/grit/app_locale_settings.h"
#endif

#if defined(OS_MACOSX)
#include "shell/browser/ui/cocoa/views_delegate_mac.h"
#else
#include "shell/browser/ui/views/electron_views_delegate.h"
#endif

#if defined(OS_LINUX)
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/dbus/dbus_bluez_manager_wrapper_linux.h"
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/browser_context_keyed_service_factories.h"
#include "extensions/common/extension_api.h"
#include "shell/browser/extensions/electron_browser_context_keyed_service_factories.h"
#include "shell/browser/extensions/electron_extensions_browser_client.h"
#include "shell/common/extensions/electron_extensions_client.h"
#endif  // BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "chrome/browser/spellchecker/spellcheck_factory.h"  // nogncheck
#endif

namespace electron {

namespace {

template <typename T>
void Erase(T* container, typename T::iterator iter) {
  container->erase(iter);
}

#if defined(OS_WIN)
// gfx::Font callbacks
void AdjustUIFont(gfx::win::FontAdjustment* font_adjustment) {
  l10n_util::NeedOverrideDefaultUIFont(&font_adjustment->font_family_override,
                                       &font_adjustment->font_scale);
  font_adjustment->font_scale *= display::win::GetAccessibilityFontScale();
}

int GetMinimumFontSize() {
  int min_font_size;
  base::StringToInt(l10n_util::GetStringUTF16(IDS_MINIMUM_UI_FONT_SIZE),
                    &min_font_size);
  return min_font_size;
}
#endif

base::string16 MediaStringProvider(media::MessageId id) {
  switch (id) {
    case media::DEFAULT_AUDIO_DEVICE_NAME:
      return base::ASCIIToUTF16("Default");
#if defined(OS_WIN)
    case media::COMMUNICATIONS_AUDIO_DEVICE_NAME:
      return base::ASCIIToUTF16("Communications");
#endif
    default:
      return base::string16();
  }
}

#if defined(USE_X11)
// Indicates that we're currently responding to an IO error (by shutting down).
bool g_in_x11_io_error_handler = false;

// Number of seconds to wait for UI thread to get an IO error if we get it on
// the background thread.
const int kWaitForUIThreadSeconds = 10;

void OverrideLinuxAppDataPath() {
  base::FilePath path;
  if (base::PathService::Get(DIR_APP_DATA, &path))
    return;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  path = base::nix::GetXDGDirectory(env.get(), base::nix::kXdgConfigHomeEnvVar,
                                    base::nix::kDotConfigDir);
  base::PathService::Override(DIR_APP_DATA, path);
}

int BrowserX11ErrorHandler(Display* d, XErrorEvent* error) {
  if (!g_in_x11_io_error_handler && base::ThreadTaskRunnerHandle::IsSet()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&ui::LogErrorEventDescription, d, *error));
  }
  return 0;
}

// This function is used to help us diagnose crash dumps that happen
// during the shutdown process.
NOINLINE void WaitingForUIThreadToHandleIOError() {
  // Ensure function isn't optimized away.
  asm("");
  sleep(kWaitForUIThreadSeconds);
}

int BrowserX11IOErrorHandler(Display* d) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    // Wait for the UI thread (which has a different connection to the X server)
    // to get the error. We can't call shutdown from this thread without
    // tripping an error. Doing it through a function so that we'll be able
    // to see it in any crash dumps.
    WaitingForUIThreadToHandleIOError();
    return 0;
  }

  // If there's an IO error it likely means the X server has gone away.
  // If this DCHECK fails, then that means SessionEnding() below triggered some
  // code that tried to talk to the X server, resulting in yet another error.
  DCHECK(!g_in_x11_io_error_handler);

  g_in_x11_io_error_handler = true;
  LOG(ERROR) << "X IO error received (X server probably went away)";
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());

  return 0;
}

int X11EmptyErrorHandler(Display* d, XErrorEvent* error) {
  return 0;
}

int X11EmptyIOErrorHandler(Display* d) {
  return 0;
}
#endif

}  // namespace

// static
ElectronBrowserMainParts* ElectronBrowserMainParts::self_ = nullptr;

ElectronBrowserMainParts::ElectronBrowserMainParts(
    const content::MainFunctionParams& params)
    : fake_browser_process_(new BrowserProcessImpl),
      browser_(new Browser),
      node_bindings_(
          NodeBindings::Create(NodeBindings::BrowserEnvironment::BROWSER)),
      electron_bindings_(new ElectronBindings(uv_default_loop())) {
  DCHECK(!self_) << "Cannot have two ElectronBrowserMainParts";
  self_ = this;
}

ElectronBrowserMainParts::~ElectronBrowserMainParts() {
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
ElectronBrowserMainParts* ElectronBrowserMainParts::Get() {
  DCHECK(self_);
  return self_;
}

bool ElectronBrowserMainParts::SetExitCode(int code) {
  if (!exit_code_)
    return false;

  *exit_code_ = code;
  return true;
}

int ElectronBrowserMainParts::GetExitCode() {
  return exit_code_ != nullptr ? *exit_code_ : 0;
}

void ElectronBrowserMainParts::RegisterDestructionCallback(
    base::OnceClosure callback) {
  // The destructors should be called in reversed order, so dependencies between
  // JavaScript objects can be correctly resolved.
  // For example WebContentsView => WebContents => Session.
  destructors_.insert(destructors_.begin(), std::move(callback));
}

int ElectronBrowserMainParts::PreEarlyInitialization() {
  field_trial_list_ = std::make_unique<base::FieldTrialList>(nullptr);
#if defined(USE_X11)
  views::LinuxUI::SetInstance(BuildGtkUi());
  OverrideLinuxAppDataPath();

  // Installs the X11 error handlers for the browser process used during
  // startup. They simply print error messages and exit because
  // we can't shutdown properly while creating and initializing services.
  ui::SetX11ErrorHandlers(nullptr, nullptr);
#endif

#if defined(OS_POSIX)
  HandleSIGCHLD();
#endif

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void ElectronBrowserMainParts::PostEarlyInitialization() {
  // A workaround was previously needed because there was no ThreadTaskRunner
  // set.  If this check is failing we may need to re-add that workaround
  DCHECK(base::ThreadTaskRunnerHandle::IsSet());

  // The ProxyResolverV8 has setup a complete V8 environment, in order to
  // avoid conflicts we only initialize our V8 environment after that.
  js_env_ = std::make_unique<JavascriptEnvironment>(node_bindings_->uv_loop());

  node_bindings_->Initialize();
  // Create the global environment.
  node::Environment* env = node_bindings_->CreateEnvironment(
      js_env_->context(), js_env_->platform());
  node_env_ = std::make_unique<NodeEnvironment>(env);

  // Enable support for v8 inspector
  node_debugger_ = std::make_unique<NodeDebugger>(env);
  node_debugger_->Start();

  // Add Electron extended APIs.
  electron_bindings_->BindTo(js_env_->isolate(), env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(env);

  // Wrap the uv loop with global env.
  node_bindings_->set_uv_env(env);

  // We already initialized the feature list in PreEarlyInitialization(), but
  // the user JS script would not have had a chance to alter the command-line
  // switches at that point. Lets reinitialize it here to pick up the
  // command-line changes.
  base::FeatureList::ClearInstanceForTesting();
  InitializeFeatureList();

  // Initialize after user script environment creation.
  fake_browser_process_->PostEarlyInitialization();
}

int ElectronBrowserMainParts::PreCreateThreads() {
#if defined(USE_AURA)
  display::Screen* screen = views::CreateDesktopScreen();
  display::Screen::SetScreenInstance(screen);
#if defined(USE_X11)
  views::LinuxUI::instance()->UpdateDeviceScaleFactor();
#endif
#endif

  if (!views::LayoutProvider::Get())
    layout_provider_ = std::make_unique<views::LayoutProvider>();

  // Initialize the app locale.
  fake_browser_process_->SetApplicationLocale(
      ElectronBrowserClient::Get()->GetApplicationLocale());

  // Force MediaCaptureDevicesDispatcher to be created on UI thread.
  MediaCaptureDevicesDispatcher::GetInstance();

  // Force MediaCaptureDevicesDispatcher to be created on UI thread.
  MediaCaptureDevicesDispatcher::GetInstance();

#if defined(OS_MACOSX)
  ui::InitIdleMonitor();
#endif

  fake_browser_process_->PreCreateThreads();

  return 0;
}

void ElectronBrowserMainParts::PostCreateThreads() {
  base::PostTask(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&tracing::TracingSamplerProfiler::CreateOnChildThread));
}

void ElectronBrowserMainParts::PostDestroyThreads() {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_browser_client_.reset();
  extensions::ExtensionsBrowserClient::Set(nullptr);
#endif
#if defined(OS_LINUX)
  device::BluetoothAdapterFactory::Shutdown();
  bluez::DBusBluezManagerWrapperLinux::Shutdown();
#endif
  fake_browser_process_->PostDestroyThreads();
}

void ElectronBrowserMainParts::ToolkitInitialized() {
  ui::MaterialDesignController::Initialize();

#if defined(USE_AURA) && defined(USE_X11)
  views::LinuxUI::instance()->Initialize();
#endif

#if defined(USE_AURA)
  wm_state_ = std::make_unique<wm::WMState>();
#endif

#if defined(OS_WIN)
  gfx::win::SetAdjustFontCallback(&AdjustUIFont);
  gfx::win::SetGetMinimumFontSizeCallback(&GetMinimumFontSize);

  wchar_t module_name[MAX_PATH] = {0};
  if (GetModuleFileName(NULL, module_name, base::size(module_name)))
    ui::CursorLoaderWin::SetCursorResourceModule(module_name);
#endif

#if defined(OS_MACOSX)
  views_delegate_.reset(new ViewsDelegateMac);
#else
  views_delegate_ = std::make_unique<ViewsDelegate>();
#endif
}

void ElectronBrowserMainParts::PreMainMessageLoopRun() {
  // Run user's main script before most things get initialized, so we can have
  // a chance to setup everything.
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_client_ = std::make_unique<ElectronExtensionsClient>();
  extensions::ExtensionsClient::Set(extensions_client_.get());

  // BrowserContextKeyedAPIServiceFactories require an ExtensionsBrowserClient.
  extensions_browser_client_ =
      std::make_unique<ElectronExtensionsBrowserClient>();
  extensions::ExtensionsBrowserClient::Set(extensions_browser_client_.get());

  extensions::EnsureBrowserContextKeyedServiceFactoriesBuilt();
  extensions::electron::EnsureBrowserContextKeyedServiceFactoriesBuilt();
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  SpellcheckServiceFactory::GetInstance();
#endif

#if defined(USE_X11)
  ui::TouchFactory::SetTouchDeviceListFromCommandLine();
#endif

  // Start idle gc.
  gc_timer_.Start(FROM_HERE, base::TimeDelta::FromMinutes(1),
                  base::BindRepeating(&v8::Isolate::LowMemoryNotification,
                                      base::Unretained(js_env_->isolate())));

  content::WebUIControllerFactory::RegisterFactory(
      ElectronWebUIControllerFactory::GetInstance());

  // --remote-debugging-port
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kRemoteDebuggingPort))
    DevToolsManagerDelegate::StartHttpHandler();

#if !defined(OS_MACOSX)
  // The corresponding call in macOS is in ElectronApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  Browser::Get()->DidFinishLaunching(base::DictionaryValue());
#endif

  // Notify observers that main thread message loop was initialized.
  Browser::Get()->PreMainMessageLoopRun();
}

bool ElectronBrowserMainParts::MainMessageLoopRun(int* result_code) {
  js_env_->OnMessageLoopCreated();
  exit_code_ = result_code;
  return content::BrowserMainParts::MainMessageLoopRun(result_code);
}

void ElectronBrowserMainParts::PreDefaultMainMessageLoopRun(
    base::OnceClosure quit_closure) {
  Browser::Get()->SetMainMessageLoopQuitClosure(std::move(quit_closure));
}

void ElectronBrowserMainParts::PostMainMessageLoopStart() {
#if defined(USE_X11)
  // Installs the X11 error handlers for the browser process after the
  // main message loop has started. This will allow us to exit cleanly
  // if X exits before us.
  ui::SetX11ErrorHandlers(BrowserX11ErrorHandler, BrowserX11IOErrorHandler);
#endif
#if defined(OS_LINUX)
  bluez::DBusBluezManagerWrapperLinux::Initialize();
#endif
#if defined(OS_POSIX)
  HandleShutdownSignals();
#endif
}

void ElectronBrowserMainParts::PostMainMessageLoopRun() {
#if defined(USE_X11)
  // Unset the X11 error handlers. The X11 error handlers log the errors using a
  // |PostTask()| on the message-loop. But since the message-loop is in the
  // process of terminating, this can cause errors.
  ui::SetX11ErrorHandlers(X11EmptyErrorHandler, X11EmptyIOErrorHandler);
#endif

  node_debugger_->Stop();
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

  fake_browser_process_->PostMainMessageLoopRun();
}

#if !defined(OS_MACOSX)
void ElectronBrowserMainParts::PreMainMessageLoopStart() {
  PreMainMessageLoopStartCommon();
}
#endif

void ElectronBrowserMainParts::PreMainMessageLoopStartCommon() {
#if defined(OS_MACOSX)
  InitializeMainNib();
  RegisterURLHandler();
#endif
  media::SetLocalizedStringProvider(MediaStringProvider);
}

device::mojom::GeolocationControl*
ElectronBrowserMainParts::GetGeolocationControl() {
  if (!geolocation_control_) {
    content::GetDeviceService().BindGeolocationControl(
        geolocation_control_.BindNewPipeAndPassReceiver());
  }
  return geolocation_control_.get();
}

IconManager* ElectronBrowserMainParts::GetIconManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!icon_manager_.get())
    icon_manager_ = std::make_unique<IconManager>();
  return icon_manager_.get();
}

}  // namespace electron
