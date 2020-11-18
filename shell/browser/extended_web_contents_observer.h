// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_
#define SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_

#include <vector>

#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "electron/shell/common/api/api.mojom.h"
#include "ui/gfx/geometry/rect.h"

namespace electron {

// Certain events are only in WebContentsDelegate, so we provide our own
// Observer to dispatch those events.
class ExtendedWebContentsObserver : public base::CheckedObserver {
 public:
  virtual void OnCloseContents() {}
  virtual void OnDraggableRegionsUpdated(
      const std::vector<mojom::DraggableRegionPtr>& regions) {}
  virtual void OnSetContentBounds(const gfx::Rect& rect) {}
  virtual void OnActivateContents() {}
  virtual void OnPageTitleUpdated(const base::string16& title,
                                  bool explicit_set) {}
  virtual void OnDevToolsResized() {}

 protected:
  ~ExtendedWebContentsObserver() override {}
};

}  // namespace electron

#endif  // SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_
