// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_
#define ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_

#include <list>
#include <map>
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

  // Returns the BrowserContext associated with the partition.
  content::BrowserContext* GetBrowserContextForPartition(
      const std::string& partition, bool in_memory);

  // Register a callback that should be destroyed before JavaScript environment
  // gets destroyed.
  void RegisterDestructionCallback(const base::Closure& callback);

  Browser* browser() { return browser_.get(); }

 protected:
  // Implementations of brightray::BrowserMainParts.
  brightray::BrowserContext* CreateBrowserContext() override;

  // Implementations of content::BrowserMainParts.
  void PostEarlyInitialization() override;
  void PreMainMessageLoopRun() override;
#if defined(OS_MACOSX)
  void PreMainMessageLoopStart() override;
  void PostDestroyThreads() override;
#endif

 private:
#if defined(USE_X11)
  void SetDPIFromGSettings();
#endif

  // A fake BrowserProcess object that used to feed the source code from chrome.
  scoped_ptr<BrowserProcess> fake_browser_process_;

  // The gin::PerIsolateData requires a task runner to create, so we feed it
  // with a task runner that will post all work to main loop.
  scoped_refptr<BridgeTaskRunner> bridge_task_runner_;

  scoped_ptr<Browser> browser_;
  scoped_ptr<JavascriptEnvironment> js_env_;
  scoped_ptr<NodeBindings> node_bindings_;
  scoped_ptr<AtomBindings> atom_bindings_;
  scoped_ptr<NodeDebugger> node_debugger_;

  base::Timer gc_timer_;

  // List of callbacks should be executed before destroying JS env.
  std::list<base::Closure> destruction_callbacks_;

  // partition_id => browser_context
  std::map<std::string, scoped_refptr<brightray::BrowserContext>>
      browser_context_map_;

  static AtomBrowserMainParts* self_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserMainParts);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_
