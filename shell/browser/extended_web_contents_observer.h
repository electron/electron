// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_
#define ELECTRON_SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"

namespace gfx {
class Rect;
}

namespace electron {

// Certain events are only in WebContentsDelegate, so we provide our own
// Observer to dispatch those events.
class ExtendedWebContentsObserver : public base::CheckedObserver {
 public:
  virtual void OnSetContentBounds(const gfx::Rect& rect) {}
  virtual void OnActivateContents() {}
  virtual void OnPageTitleUpdated(const std::u16string& title,
                                  bool explicit_set) {}
  virtual void OnDevToolsResized() {}

 protected:
  ~ExtendedWebContentsObserver() override {}
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_
