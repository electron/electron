// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_BROWSER_MAIN_PARTS_H_
#define BRIGHTRAY_BROWSER_BROWSER_MAIN_PARTS_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "brightray/browser/brightray_paths.h"
#include "content/public/browser/browser_main_parts.h"
#include "ui/views/layout/layout_provider.h"

#if defined(USE_AURA)
namespace wm {
class WMState;
}
#endif

namespace brightray {

class BrowserMainParts : public content::BrowserMainParts {
 public:
  BrowserMainParts();
  ~BrowserMainParts() override;

 protected:
  // content::BrowserMainParts:
  bool ShouldContentCreateFeatureList() override;
  int PreEarlyInitialization() override;
  void ToolkitInitialized() override;
  void PreMainMessageLoopStart() override;
  void PreMainMessageLoopRun() override;
  void PostMainMessageLoopStart() override;
  void PostMainMessageLoopRun() override;
  int PreCreateThreads() override;
  void PostDestroyThreads() override;

  void InitializeFeatureList();

 private:
#if defined(OS_MACOSX)
  void InitializeMainNib();
  void OverrideAppLogsPath();
#endif

#if defined(USE_AURA)
  std::unique_ptr<wm::WMState> wm_state_;
#endif

  std::unique_ptr<views::LayoutProvider> layout_provider_;
  std::string custom_locale_;

  DISALLOW_COPY_AND_ASSIGN(BrowserMainParts);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_BROWSER_MAIN_PARTS_H_
