// Copyright 2024 Microsoft GmbH.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_OPAQUE_FRAME_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_OPAQUE_FRAME_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/view_ids.h"
#include "shell/browser/ui/views/frameless_view.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/linux/window_button_order_observer.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/window/caption_button_types.h"
#include "ui/views/window/frame_buttons.h"
#include "ui/views/window/non_client_view.h"

class CaptionButtonPlaceholderContainer;

namespace electron {

class NativeWindowViews;

class OpaqueFrameView : public FramelessView {
  METADATA_HEADER(OpaqueFrameView, FramelessView)

 public:
  // Constants used by OpaqueBrowserFrameView as well.
  static const int kContentEdgeShadowThickness;

  static constexpr int kNonClientExtraTopThickness = 1;

  OpaqueFrameView();
  ~OpaqueFrameView() override;

  // FramelessView:
  void Init(NativeWindowViews* window, views::Widget* frame) override;
  int ResizingBorderHitTest(const gfx::Point& point) override;
  void InvalidateCaptionButtons() override;

  // views::NonClientFrameView:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void ResetWindowControls() override;
  views::View* TargetForRect(views::View* root, const gfx::Rect& rect) override;

  // views::View:
  void Layout(PassKey) override;
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  enum ButtonAlignment { ALIGN_LEADING, ALIGN_TRAILING };

  struct TopAreaPadding {
    int leading;
    int trailing;
  };

  void PaintAsActiveChanged();

  void UpdateCaptionButtonPlaceholderContainerBackground();
  void UpdateFrameCaptionButtons();
  void LayoutWindowControls();
  void LayoutWindowControlsOverlay();

  // Creates and returns an ImageButton with |this| as its listener.
  // Memory is owned by the caller.
  views::Button* CreateButton(ViewID view_id,
                              int accessibility_string_id,
                              views::CaptionButtonIcon icon_type,
                              int ht_component,
                              const gfx::VectorIcon& icon_image,
                              views::Button::PressedCallback callback);

  /** Layout-Related Utility Functions **/

  // Returns the insets from the native window edge to the client view.
  // This does not include any client edge.  If |restored| is true, this
  // is calculated as if the window was restored, regardless of its
  // current node_data.
  gfx::Insets FrameBorderInsets(bool restored) const;

  // Returns the thickness of the border that makes up the window frame edge
  // along the top of the frame. If |restored| is true, this acts as if the
  // window is restored regardless of the actual mode.
  int FrameTopBorderThickness(bool restored) const;

  // Returns the spacing between the edge of the browser window and the first
  // frame buttons.
  TopAreaPadding GetTopAreaPadding(bool has_leading_buttons,
                                   bool has_trailing_buttons) const;

  // Determines whether the top frame is condensed vertically, as when the
  // window is maximized. If true, the top frame is just the height of a tab,
  // rather than having extra vertical space above the tabs.
  bool IsFrameCondensed() const;

  // The insets from the native window edge to the client view when the window
  // is restored.  This goes all the way to the web contents on the left, right,
  // and bottom edges.
  gfx::Insets RestoredFrameBorderInsets() const;

  // The insets from the native window edge to the flat portion of the
  // window border.  That is, this function returns the "3D portion" of the
  // border when the window is restored.  The returned insets will not be larger
  // than RestoredFrameBorderInsets().
  gfx::Insets RestoredFrameEdgeInsets() const;

  // Additional vertical padding between tabs and the top edge of the window
  // when the window is restored.
  int NonClientExtraTopThickness() const;

  // Returns the height of the entire nonclient top border, from the edge of the
  // window to the top of the tabs. If |restored| is true, this is calculated as
  // if the window was restored, regardless of its current state.
  int NonClientTopHeight(bool restored) const;

  // Returns the y-coordinate of button |button_id|.  If |restored| is true,
  // acts as if the window is restored regardless of the real mode.
  int CaptionButtonY(views::FrameButton button_id, bool restored) const;

  // Returns the y-coordinate of the caption button when native frame buttons
  // are disabled.  If |restored| is true, acts as if the window is restored
  // regardless of the real mode.
  int DefaultCaptionButtonY(bool restored) const;

  // Returns the insets from the native window edge to the flat portion of the
  // window border.  That is, this function returns the "3D portion" of the
  // border.  If |restored| is true, acts as if the window is restored
  // regardless of the real mode.
  gfx::Insets FrameEdgeInsets(bool restored) const;

  // Returns the size of the window icon. This can be platform dependent
  // because of differences in fonts.
  int GetIconSize() const;

  // Returns the spacing between the edge of the browser window and the first
  // frame buttons.
  TopAreaPadding GetTopAreaPadding() const;

  // Returns the color of the frame.
  SkColor GetFrameColor() const;

  // Initializes the button with |button_id| to be aligned according to
  // |alignment|.
  void ConfigureButton(views::FrameButton button_id, ButtonAlignment alignment);

  // Sets the visibility of all buttons associated with |button_id| to false.
  void HideButton(views::FrameButton button_id);

  // Adds a window caption button to either the leading or trailing side.
  void SetBoundsForButton(views::FrameButton button_id,
                          views::Button* button,
                          ButtonAlignment alignment);

  // Computes the height of the top area of the frame.
  int GetTopAreaHeight() const;

  // Returns the margin around button |button_id|.  If |leading_spacing| is
  // true, returns the left margin (in RTL), otherwise returns the right margin
  // (in RTL).  Extra margin may be added if |is_leading_button| is true.
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
