// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_BROWSER_MAIN_PARTS_H_
#define SHELL_BROWSER_ELECTRON_BROWSER_MAIN_PARTS_H_

#include <list>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/metrics/field_trial.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_main_parts.h"
#include "content/public/common/main_function_params.h"
#include "electron/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/geolocation_control.mojom.h"
#include "ui/views/layout/layout_provider.h"

class BrowserProcess;
class IconManager;

#if defined(USE_AURA)
namespace wm {
class WMState;
}
#endif

#if defined(USE_X11)
namespace ui {
class GtkUiDelegate;
}
#endif

namespace electron {

class ElectronBrowserContext;
class Browser;
class ElectronBindings;
class JavascriptEnvironment;
class NodeBindings;
class NodeDebugger;
class NodeEnvironment;
class BridgeTaskRunner;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
class ElectronExtensionsClient;
class ElectronExtensionsBrowserClient;
#endif

#if defined(TOOLKIT_VIEWS)
class ViewsDelegate;
#endif

#if defined(OS_MACOSX)
class ViewsDelegateMac;
#endif

#if defined(USE_X11)
class DarkThemeObserver;
#endif

class ElectronBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit ElectronBrowserMainParts(const content::MainFunctionParams& params);
  ~ElectronBrowserMainParts() override;

  static ElectronBrowserMainParts* Get();

  // Sets the exit code, will fail if the message loop is not ready.
  bool SetExitCode(int code);

  // Gets the exit code
  int GetExitCode();

  // Register a callback that should be destroyed before JavaScript environment
  // gets destroyed.
  // Returns a closure that can be used to remove |callback| from the list.
  void RegisterDestructionCallback(base::OnceClosure callback);

  // Returns the connection to GeolocationControl which can be
  // used to enable the location services once per client.
  device::mojom::GeolocationControl* GetGeolocationControl();

  // Returns handle to the class responsible for extracting file icons.
  IconManager* GetIconManager();

  Browser* browser() { return browser_.get(); }
  BrowserProcessImpl* browser_process() { return fake_browser_process_.get(); }

 protected:
  // content::BrowserMainParts:
  int PreEarlyInitialization() override;
  void PostEarlyInitialization() override;
  int PreCreateThreads() override;
  void ToolkitInitialized() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PreDefaultMainMessageLoopRun(base::OnceClosure quit_closure) override;
  void PostMainMessageLoopStart() override;
  void PostMainMessageLoopRun() override;
  void PreMainMessageLoopStart() override;
  void PostCreateThreads() override;
  void PostDestroyThreads() override;

 private:
  void PreMainMessageLoopStartCommon();

#if defined(OS_POSIX)
  // Set signal handlers.
  void HandleSIGCHLD();
  void HandleShutdownSignals();
#endif

#if defined(OS_MACOSX)
  void FreeAppDelegate();
  void RegisterURLHandler();
  void InitializeMainNib();
#endif

#if defined(OS_MACOSX)
  std::unique_ptr<ViewsDelegateMac> views_delegate_;
#else
  std::unique_ptr<ViewsDelegate> views_delegate_;
#endif

#if defined(USE_AURA)
  std::unique_ptr<wm::WMState> wm_state_;
#endif

#if defined(USE_X11)
  std::unique_ptr<ui::GtkUiDelegate> gtk_ui_delegate_;
  // Used to notify the native theme of changes to dark mode.
  std::unique_ptr<DarkThemeObserver> dark_theme_observer_;
#endif

  std::unique_ptr<views::LayoutProvider> layout_provider_;

  // A fake BrowserProcess object that used to feed the source code from chrome.
  std::unique_ptr<BrowserProcessImpl> fake_browser_process_;

  // Pointer to exit code.
  int* exit_code_ = nullptr;

  std::unique_ptr<JavascriptEnvironment> js_env_;
  std::unique_ptr<Browser> browser_;
  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<ElectronBindings> electron_bindings_;
  std::unique_ptr<NodeEnvironment> node_env_;
  std::unique_ptr<NodeDebugger> node_debugger_;
  std::unique_ptr<IconManager> icon_manager_;
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::unique_ptr<ElectronExtensionsClient> extensions_client_;
  std::unique_ptr<ElectronExtensionsBrowserClient> extensions_browser_client_;
#endif

  base::RepeatingTimer gc_timer_;

  // List of callbacks should be executed before destroying JS env.
  std::list<base::OnceClosure> destructors_;

  mojo::Remote<device::mojom::GeolocationControl> geolocation_control_;

  static ElectronBrowserMainParts* self_;

  DISALLOW_COPY_AND_ASSIGN(ElectronBrowserMainParts);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_BROWSER_MAIN_PARTS_H_
