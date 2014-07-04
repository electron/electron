// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_BROWSER_MAIN_PARTS_H_
#define BRIGHTRAY_BROWSER_BROWSER_MAIN_PARTS_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/browser_main_parts.h"

#if defined(TOOLKIT_VIEWS)
namespace brightray {
class ViewsDelegate;
}

namespace wm {
class WMState;
}
#endif

namespace brightray {

class BrowserContext;
class WebUIControllerFactory;

class BrowserMainParts : public content::BrowserMainParts {
 public:
  BrowserMainParts();
  ~BrowserMainParts();

  BrowserContext* browser_context() { return browser_context_.get(); }

 protected:
  // content::BrowserMainParts:
  virtual void PreEarlyInitialization() OVERRIDE;
  virtual void ToolkitInitialized() OVERRIDE;
  virtual void PreMainMessageLoopStart() OVERRIDE;
  virtual void PreMainMessageLoopRun() OVERRIDE;
  virtual void PostMainMessageLoopRun() OVERRIDE;
  virtual int PreCreateThreads() OVERRIDE;

  // Subclasses should override this to provide their own BrowserContxt
  // implementation. The caller takes ownership of the returned object.
  virtual BrowserContext* CreateBrowserContext();

  // Override this to change how ProxyResolverV8 is initialized.
  virtual void InitProxyResolverV8();

 private:
#if defined(OS_MACOSX)
  void IncreaseFileDescriptorLimit();
  void InitializeMainNib();
#endif

  scoped_ptr<BrowserContext> browser_context_;
  scoped_ptr<WebUIControllerFactory> web_ui_controller_factory_;

#if defined(TOOLKIT_VIEWS)
  scoped_ptr<ViewsDelegate> views_delegate_;
  scoped_ptr<wm::WMState> wm_state_;
#endif

  DISALLOW_COPY_AND_ASSIGN(BrowserMainParts);
};

}  // namespace brightray

#endif
