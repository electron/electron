// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_MAIN_PARTS_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_MAIN_PARTS_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/task/single_thread_task_runner.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_main_parts.h"
#include "electron/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/geolocation_control.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/display/screen.h"
#include "ui/views/layout/layout_provider.h"

class BrowserProcessImpl;
class IconManager;

namespace base {
class FieldTrialList;
}

#if defined(USE_AURA)
namespace wm {
class WMState;
}

namespace display {
class Screen;
}
#endif

namespace node {
class Environment;
}

namespace ui {
class LinuxUiGetter;
class DarkModeManagerLinux;
}  // namespace ui

namespace electron {

class Browser;
class ElectronBindings;
class JavascriptEnvironment;
class NodeBindings;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
class ElectronExtensionsClient;
class ElectronExtensionsBrowserClient;
#endif

#if defined(TOOLKIT_VIEWS)
class ViewsDelegate;
#endif

#if BUILDFLAG(IS_MAC)
class ViewsDelegateMac;
#endif

class ElectronBrowserMainParts : public content::BrowserMainParts {
 public:
  ElectronBrowserMainParts();
  ~ElectronBrowserMainParts() override;

  // disable copy
  ElectronBrowserMainParts(const ElectronBrowserMainParts&) = delete;
  ElectronBrowserMainParts& operator=(const ElectronBrowserMainParts&) = delete;

  static ElectronBrowserMainParts* Get();

  // Sets the exit code, will fail if the message loop is not ready.
  bool SetExitCode(int code);

  // Gets the exit code
  int GetExitCode() const;

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
  int PreMainMessageLoopRun() override;
  void WillRunMainMessageLoop(
      std::unique_ptr<base::RunLoop>& run_loop) override;
  void PostCreateMainMessageLoop() override;
  void PostMainMessageLoopRun() override;
  void PreCreateMainMessageLoop() override;
  void PostCreateThreads() override;
  void PostDestroyThreads() override;

 private:
  void PreCreateMainMessageLoopCommon();

#if BUILDFLAG(IS_POSIX)
  // Set signal handlers.
  void HandleSIGCHLD();
  void InstallShutdownSignalHandlers(
      base::OnceCallback<void()> shutdown_callback,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
#endif

#if BUILDFLAG(IS_LINUX)
  void DetectOzonePlatform();
#endif

#if BUILDFLAG(IS_MAC)
  void FreeAppDelegate();
  void RegisterURLHandler();
  void InitializeMainNib();
  static std::string GetCurrentSystemLocale();
#endif

#if BUILDFLAG(IS_MAC)
  std::unique_ptr<ViewsDelegateMac> views_delegate_;
#else
  std::unique_ptr<ViewsDelegate> views_delegate_;
#endif

#if defined(USE_AURA)
  std::unique_ptr<wm::WMState> wm_state_;
  std::unique_ptr<display::Screen> screen_;
#endif

#if BUILDFLAG(IS_LINUX)
  std::unique_ptr<ui::DarkModeManagerLinux> dark_mode_manager_;
  std::unique_ptr<ui::LinuxUiGetter> linux_ui_getter_;
#endif

  std::unique_ptr<views::LayoutProvider> layout_provider_;

  // A fake BrowserProcess object that used to feed the source code from chrome.
  std::unique_ptr<BrowserProcessImpl> fake_browser_process_;

  // A place to remember the exit code once the message loop is ready.
  // Before then, we just exit() without any intermediate steps.
  absl::optional<int> exit_code_;

  std::unique_ptr<NodeBindings> node_bindings_;

  // depends-on: node_bindings_
  std::unique_ptr<ElectronBindings> electron_bindings_;

  // depends-on: node_bindings_
  std::unique_ptr<JavascriptEnvironment> js_env_;

  // depends-on: js_env_'s isolate
  std::shared_ptr<node::Environment> node_env_;

  // depends-on: js_env_'s isolate
  std::unique_ptr<Browser> browser_;

  std::unique_ptr<IconManager> icon_manager_;
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::unique_ptr<ElectronExtensionsClient> extensions_client_;
  std::unique_ptr<ElectronExtensionsBrowserClient> extensions_browser_client_;
#endif

  mojo::Remote<device::mojom::GeolocationControl> geolocation_control_;

#if BUILDFLAG(IS_MAC)
  std::unique_ptr<display::ScopedNativeScreen> screen_;
#endif

  static ElectronBrowserMainParts* self_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_MAIN_PARTS_H_
