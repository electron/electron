// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <list>
#include <string>

#include "base/callback.h"
#include "base/timer/timer.h"
#include "brightray/browser/browser_main_parts.h"
#include "content/public/browser/browser_context.h"

class BrowserProcess;

namespace atom {

class AtomBindings;
class Browser;
class JavascriptEnvironment;
class NodeBindings;
class NodeDebugger;
class BridgeTaskRunner;

class AtomBrowserMainParts : public brightray::BrowserMainParts {
 public:
  AtomBrowserMainParts();
  virtual ~AtomBrowserMainParts();

  static AtomBrowserMainParts* Get();

  // Sets the exit code, will fail if the message loop is not ready.
  bool SetExitCode(int code);

  // Gets the exit code
  int GetExitCode();

  // Register a callback that should be destroyed before JavaScript environment
  // gets destroyed.
  // Returns a closure that can be used to remove |callback| from the list.
  base::Closure RegisterDestructionCallback(const base::Closure& callback);

  Browser* browser() { return browser_.get(); }

 protected:
  // content::BrowserMainParts:
  void PreEarlyInitialization() override;
  void PostEarlyInitialization() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostMainMessageLoopStart() override;
  void PostMainMessageLoopRun() override;
#if defined(OS_MACOSX)
  void PreMainMessageLoopStart() override;
#endif

 private:
#if defined(OS_POSIX)
  // Set signal handlers.
  void HandleSIGCHLD();
  void HandleShutdownSignals();
#endif

#if defined(OS_MACOSX)
  void FreeAppDelegate();
#endif

  // A fake BrowserProcess object that used to feed the source code from chrome.
  std::unique_ptr<BrowserProcess> fake_browser_process_;

  // The gin::PerIsolateData requires a task runner to create, so we feed it
  // with a task runner that will post all work to main loop.
  scoped_refptr<BridgeTaskRunner> bridge_task_runner_;

  // Pointer to exit code.
  int* exit_code_;

  std::unique_ptr<Browser> browser_;
  std::unique_ptr<JavascriptEnvironment> js_env_;
  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<AtomBindings> atom_bindings_;
  std::unique_ptr<NodeDebugger> node_debugger_;

  base::Timer gc_timer_;

  // List of callbacks should be executed before destroying JS env.
  std::list<base::Closure> destructors_;

  static AtomBrowserMainParts* self_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserMainParts);
};

}  // namespace atom
