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
#endif

#if defined(USE_AURA) && defined(USE_X11)
namespace wm {
class WMState;
}
#endif

namespace brightray {

class BrowserContext;
class WebUIControllerFactory;
class RemoteDebuggingServer;

class BrowserMainParts : public content::BrowserMainParts {
 public:
  BrowserMainParts();
  ~BrowserMainParts();

  BrowserContext* browser_context() { return browser_context_.get(); }

 protected:
  // content::BrowserMainParts:
  virtual void PreEarlyInitialization() override;
  virtual void ToolkitInitialized() override;
  virtual void PreMainMessageLoopStart() override;
  virtual void PreMainMessageLoopRun() override;
  virtual void PostMainMessageLoopRun() override;
  virtual int PreCreateThreads() override;

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
  scoped_ptr<RemoteDebuggingServer> remote_debugging_server_;

#if defined(TOOLKIT_VIEWS)
  scoped_ptr<ViewsDelegate> views_delegate_;
#endif

#if defined(USE_AURA) && defined(USE_X11)
  scoped_ptr<wm::WMState> wm_state_;
#endif

  DISALLOW_COPY_AND_ASSIGN(BrowserMainParts);
};

}  // namespace brightray

#endif
