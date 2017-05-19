// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_BROWSER_MAIN_PARTS_H_
#define BRIGHTRAY_BROWSER_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/browser_main_parts.h"

#if defined(TOOLKIT_VIEWS)
namespace brightray {
class ViewsDelegate;
}
#endif

#if defined(USE_AURA)
namespace wm {
class WMState;
}
#endif

namespace brightray {

class BrowserMainParts : public content::BrowserMainParts {
 public:
  BrowserMainParts();
  ~BrowserMainParts();

 protected:
  // content::BrowserMainParts:
  void PreEarlyInitialization() override;
  void ToolkitInitialized() override;
  void PreMainMessageLoopStart() override;
  void PreMainMessageLoopRun() override;
  void PostMainMessageLoopStart() override;
  void PostMainMessageLoopRun() override;
  int PreCreateThreads() override;
  void PostDestroyThreads() override;

 private:
#if defined(OS_MACOSX)
  void InitializeMainNib();
#endif

#if defined(TOOLKIT_VIEWS)
  std::unique_ptr<ViewsDelegate> views_delegate_;
#endif

#if defined(USE_AURA)
  std::unique_ptr<wm::WMState> wm_state_;
#endif

  DISALLOW_COPY_AND_ASSIGN(BrowserMainParts);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_BROWSER_MAIN_PARTS_H_
