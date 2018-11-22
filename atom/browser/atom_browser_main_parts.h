// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_
#define ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_

#include <list>
#include <string>

#include "base/callback.h"
#include "base/timer/timer.h"
#include "brightray/browser/browser_main_parts.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/main_function_params.h"
#include "services/device/public/mojom/geolocation_control.mojom.h"

class BrowserProcess;

#if defined(TOOLKIT_VIEWS)
namespace brightray {
class ViewsDelegate;
}
#endif

namespace net_log {
class ChromeNetLog;
}

namespace atom {

class AtomBindings;
class Browser;
class IOThread;
class JavascriptEnvironment;
class NodeBindings;
class NodeDebugger;
class NodeEnvironment;
class BridgeTaskRunner;

#if defined(OS_MACOSX)
class ViewsDelegateMac;
#endif

class AtomBrowserMainParts : public brightray::BrowserMainParts {
 public:
  explicit AtomBrowserMainParts(const content::MainFunctionParams& params);
  ~AtomBrowserMainParts() override;

  static AtomBrowserMainParts* Get();

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

  Browser* browser() { return browser_.get(); }
  IOThread* io_thread() const { return io_thread_.get(); }
  net_log::ChromeNetLog* net_log() { return net_log_.get(); }

 protected:
  // content::BrowserMainParts:
  int PreEarlyInitialization() override;
  void PostEarlyInitialization() override;
  int PreCreateThreads() override;
  void ToolkitInitialized() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostMainMessageLoopStart() override;
  void PostMainMessageLoopRun() override;
#if defined(OS_MACOSX)
  void PreMainMessageLoopStart() override;
#endif
  void PostDestroyThreads() override;

 private:
#if defined(OS_POSIX)
  // Set signal handlers.
  void HandleSIGCHLD();
  void HandleShutdownSignals();
#endif

#if defined(OS_MACOSX)
  void FreeAppDelegate();
#endif

#if defined(OS_MACOSX)
  std::unique_ptr<ViewsDelegateMac> views_delegate_;
#else
  std::unique_ptr<brightray::ViewsDelegate> views_delegate_;
#endif

  // A fake BrowserProcess object that used to feed the source code from chrome.
  std::unique_ptr<BrowserProcess> fake_browser_process_;

  // The gin::PerIsolateData requires a task runner to create, so we feed it
  // with a task runner that will post all work to main loop.
  scoped_refptr<BridgeTaskRunner> bridge_task_runner_;

  // Pointer to exit code.
  int* exit_code_ = nullptr;

  std::unique_ptr<Browser> browser_;
  std::unique_ptr<JavascriptEnvironment> js_env_;
  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<AtomBindings> atom_bindings_;
  std::unique_ptr<NodeEnvironment> node_env_;
  std::unique_ptr<NodeDebugger> node_debugger_;
  std::unique_ptr<IOThread> io_thread_;
  std::unique_ptr<net_log::ChromeNetLog> net_log_;

  base::Timer gc_timer_;

  // List of callbacks should be executed before destroying JS env.
  std::list<base::OnceClosure> destructors_;

  device::mojom::GeolocationControlPtr geolocation_control_;

  const content::MainFunctionParams main_function_params_;

  static AtomBrowserMainParts* self_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserMainParts);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_
