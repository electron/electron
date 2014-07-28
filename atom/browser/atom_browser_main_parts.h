// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_
#define ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_

#include "brightray/browser/browser_main_parts.h"

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
  virtual brightray::BrowserContext* CreateBrowserContext() OVERRIDE;

  // Implementations of content::BrowserMainParts.
  virtual void PostEarlyInitialization() OVERRIDE;
  virtual void PreMainMessageLoopRun() OVERRIDE;
#if defined(OS_MACOSX)
  virtual void PreMainMessageLoopStart() OVERRIDE;
  virtual void PostDestroyThreads() OVERRIDE;
#endif

 private:
  scoped_ptr<Browser> browser_;
  scoped_ptr<JavascriptEnvironment> js_env_;
  scoped_ptr<NodeBindings> node_bindings_;
  scoped_ptr<AtomBindings> atom_bindings_;

  static AtomBrowserMainParts* self_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserMainParts);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_H_
