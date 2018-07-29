// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ATOM_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_
#define ATOM_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_

#include "atom/browser/ui/inspectable_web_contents_view.h"

#include "base/mac/scoped_nsobject.h"

@class AtomInspectableWebContentsView;

namespace atom {

class InspectableWebContentsImpl;

class InspectableWebContentsViewMac : public InspectableWebContentsView {
 public:
  explicit InspectableWebContentsViewMac(
      InspectableWebContentsImpl* inspectable_web_contents_impl);
  ~InspectableWebContentsViewMac() override;

  gfx::NativeView GetNativeView() const override;
  void ShowDevTools(bool activate) override;
  void CloseDevTools() override;
  bool IsDevToolsViewShowing() override;
  bool IsDevToolsViewFocused() override;
  void SetIsDocked(bool docked, bool activate) override;
  void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) override;
  void SetTitle(const base::string16& title) override;

  InspectableWebContentsImpl* inspectable_web_contents() {
    return inspectable_web_contents_;
  }

 private:
  // Owns us.
  InspectableWebContentsImpl* inspectable_web_contents_;

  base::scoped_nsobject<AtomInspectableWebContentsView> view_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewMac);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_
