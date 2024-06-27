// Copyright 2024 Microsoft GmbH.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_OPAQUE_FRAME_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_OPAQUE_FRAME_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/view_ids.h"
#include "shell/browser/ui/views/frameless_view.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/font_list.h"
#include "ui/linux/nav_button_provider.h"
#include "ui/linux/window_button_order_observer.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/window/caption_button_types.h"
#include "ui/views/window/frame_buttons.h"
#include "ui/views/window/non_client_view.h"

class CaptionButtonPlaceholderContainer;

namespace electron {

class OpaqueFrameView : public FramelessView {
  METADATA_HEADER(OpaqueFrameView, FramelessView)

 public:
  // Constants used by OpaqueBrowserFrameView as well.
  static const int kContentEdgeShadowThickness;

  // The frame border is only visible in restored mode and is hardcoded to 4
  // px on each side regardless of the system window border size.  This is
  // overridable by subclasses, so RestoredFrameBorderInsets() should be used
  // instead of using this constant directly.
  static constexpr int kFrameBorderThickness = 4;

  static constexpr int kNonClientExtraTopThickness = 1;

  OpaqueFrameView();
  ~OpaqueFrameView() override;

  void Init(NativeWindowViews* window, views::Widget* frame) override;
  int ResizingBorderHitTest(const gfx::Point& point) override;

  // views::NonClientFrameView:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void ResetWindowControls() override;
  views::View* TargetForRect(views::View* root, const gfx::Rect& rect) override;

  // views::View
  void OnPaint(gfx::Canvas* canvas) override;

  void UpdateCaptionButtonPlaceholderContainerBackground();
  void LayoutWindowControlsOverlay();

 protected:
  void PaintAsActiveChanged();

  // views::View:
  void Layout(PassKey) override;

 private:
  enum ButtonAlignment { ALIGN_LEADING, ALIGN_TRAILING };

  struct TopAreaPadding {
    int leading;
    int trailing;
  };
  // Creates and returns an ImageButton with |this| as its listener.
  // Memory is owned by the caller.
  views::Button* CreateButton(ViewID view_id,
                              int accessibility_string_id,
                              views::CaptionButtonIcon icon_type,
                              int ht_component,
                              const gfx::VectorIcon& icon_image,
                              views::Button::PressedCallback callback);

  // Returns the insets from the native window edge to the client view.
  // This does not include any client edge.  If |restored| is true, this
  // is calculated as if the window was restored, regardless of its
  // current node_data.
  gfx::Insets FrameBorderInsets(bool restored) const;

  // Returns the thickness of the border that makes up the window frame edge
  // along the top of the frame. If |restored| is true, this acts as if the
  // window is restored regardless of the actual mode.
  int FrameTopBorderThickness(bool restored) const;

  TopAreaPadding GetTopAreaPadding(bool has_leading_buttons,
                                   bool has_trailing_buttons) const;

  bool IsFrameCondensed() const;
  gfx::Insets RestoredFrameBorderInsets() const;
  gfx::Insets RestoredFrameEdgeInsets() const;
  int NonClientExtraTopThickness() const;
  int NonClientTopHeight(bool restored) const;

  gfx::Insets FrameEdgeInsets(bool restored) const;
  int CaptionButtonY(views::FrameButton button_id, bool restored) const;
  int DefaultCaptionButtonY(bool restored) const;
  int GetIconSize() const;
  TopAreaPadding GetTopAreaPadding() const;

  SkColor GetFrameColor() const;

  void LayoutWindowControls();

  void ConfigureButton(views::FrameButton button_id, ButtonAlignment alignment);
  void HideButton(views::FrameButton button_id);
  void SetBoundsForButton(views::FrameButton button_id,
                          views::Button* button,
                          ButtonAlignment alignment);
  int GetTopAreaHeight() const;
  int GetWindowCaptionSpacing(views::FrameButton button_id,
                              bool leading_spacing,
                              bool is_leading_button) const;

  // Window controls.
  raw_ptr<views::Button> minimize_button_;
  raw_ptr<views::Button> maximize_button_;
  raw_ptr<views::Button> restore_button_;
  raw_ptr<views::Button> close_button_;

  // The leading and trailing x positions of the empty space available for
  // laying out titlebar elements.
  int available_space_leading_x_ = 0;
  int available_space_trailing_x_ = 0;

  // Whether any of the window control buttons were packed on the leading or
  // trailing sides.  This state is only valid while layout is being performed.
  bool placed_leading_button_ = false;
  bool placed_trailing_button_ = false;

  // The size of the window buttons. This does not count labels or other
  // elements that should be counted in a minimal frame.
  int minimum_size_for_buttons_ = 0;

  std::vector<views::FrameButton> leading_buttons_;
  std::vector<views::FrameButton> trailing_buttons_{
      views::FrameButton::kMinimize, views::FrameButton::kMaximize,
      views::FrameButton::kClose};

  base::CallbackListSubscription paint_as_active_changed_subscription_;

  // PlaceholderContainer beneath the controls button for WCO.
  raw_ptr<CaptionButtonPlaceholderContainer>
      caption_button_placeholder_container_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_OPAQUE_FRAME_VIEW_H_
