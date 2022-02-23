// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified from
// chrome/browser/ui/views/frame/glass_browser_caption_button_container.h
#ifndef SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_CONTAINER_H_
#define SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_CONTAINER_H_

#include "base/scoped_observation.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace electron {

class WinFrameView;
class WinCaptionButton;

// Provides a container for Windows 10 caption buttons that can be moved between
// frame and browser window as needed. When extended horizontally, becomes a
// grab bar for moving the window.
class WinCaptionButtonContainer : public views::View,
                                  public views::WidgetObserver {
 public:
  explicit WinCaptionButtonContainer(WinFrameView* frame_view);
  ~WinCaptionButtonContainer() override;

  // Tests to see if the specified |point| (which is expressed in this view's
  // coordinates and which must be within this view's bounds) is within one of
  // the caption buttons. Returns one of HitTestCompat enum defined in
  // ui/base/hit_test.h, HTCAPTION if the area hit would be part of the window's
  // drag handle, and HTNOWHERE otherwise.
  // See also ClientView::NonClientHitTest.
  int NonClientHitTest(const gfx::Point& point) const;

  gfx::Size GetButtonSize() const;
  void SetButtonSize(gfx::Size size);

 private:
  // views::View:
  void AddedToWidget() override;
  void RemovedFromWidget() override;

  // views::WidgetObserver:
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  void ResetWindowControls();

  // Sets caption button visibility and enabled state based on window state.
  // Only one of maximize or restore button should ever be visible at the same
  // time, and both are disabled in tablet UI mode.
  void UpdateButtons();

  WinFrameView* const frame_view_;
  WinCaptionButton* const minimize_button_;
  WinCaptionButton* const maximize_button_;
  WinCaptionButton* const restore_button_;
  WinCaptionButton* const close_button_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};

  base::CallbackListSubscription subscription_ =
      ui::TouchUiController::Get()->RegisterCallback(
          base::BindRepeating(&WinCaptionButtonContainer::UpdateButtons,
                              base::Unretained(this)));
};
}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_CONTAINER_H_
