// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_
#define ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_

#include "base/timer/timer.h"
#include "brightray/browser/browser_main_parts.h"

class BrowserProcess;

namespace atom {

class AtomBindings;
class Browser;
class JavascriptEnvironment;
class NodeBindings;

class AtomBrowserMainParts : public brightray::BrowserMainParts {
 public:
  AtomBrowserMainParts();
  virtual ~AtomBrowserMainParts();

  static AtomBrowserMainParts* Get();

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

  scoped_ptr<Browser> browser_;
  scoped_ptr<JavascriptEnvironment> js_env_;
  scoped_ptr<NodeBindings> node_bindings_;
  scoped_ptr<AtomBindings> atom_bindings_;

  base::Timer gc_timer_;

  static AtomBrowserMainParts* self_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserMainParts);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_
