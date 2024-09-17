// Copyright (c) 2024 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/opaque_frame_view.h"

#include "base/containers/adapters.h"
#include "base/i18n/rtl.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/caption_button_placeholder_container.h"
#include "ui/base/hit_test.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/font_list.h"
#include "ui/linux/linux_ui.h"
#include "ui/views/background.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/window/frame_caption_button.h"
#include "ui/views/window/vector_icons/vector_icons.h"

namespace electron {

namespace {

// These values should be the same as Chromium uses.
constexpr int kCaptionButtonHeight = 18;

bool HitTestCaptionButton(views::Button* button, const gfx::Point& point) {
  return button && button->GetVisible() &&
         button->GetMirroredBounds().Contains(point);
}

// The frame has a 2 px 3D edge along the top.  This is overridable by
// subclasses, so RestoredFrameEdgeInsets() should be used instead of using this
// constant directly.
const int kTopFrameEdgeThickness = 2;

// The frame has a 1 px 3D edge along the side.  This is overridable by
// subclasses, so RestoredFrameEdgeInsets() should be used instead of using this
// constant directly.
const int kSideFrameEdgeThickness = 1;

// The minimum vertical padding between the bottom of the caption buttons and
// the top of the content shadow.
const int kCaptionButtonBottomPadding = 3;

}  // namespace

// The content edge images have a shadow built into them.
const int OpaqueFrameView::kContentEdgeShadowThickness = 2;

OpaqueFrameView::OpaqueFrameView() = default;
OpaqueFrameView::~OpaqueFrameView() = default;

void OpaqueFrameView::Init(NativeWindowViews* window, views::Widget* frame) {
  FramelessView::Init(window, frame);

  if (!window->IsWindowControlsOverlayEnabled())
    return;

  caption_button_placeholder_container_ =
      AddChildView(std::make_unique<CaptionButtonPlaceholderContainer>());

  minimize_button_ = CreateButton(
      VIEW_ID_MINIMIZE_BUTTON, IDS_ACCNAME_MINIMIZE,
      views::CAPTION_BUTTON_ICON_MINIMIZE, HTMINBUTTON,
      views::kWindowControlMinimizeIcon,
      base::BindRepeating(&views::Widget::Minimize, base::Unretained(frame)));
  maximize_button_ = CreateButton(
      VIEW_ID_MAXIMIZE_BUTTON, IDS_ACCNAME_MAXIMIZE,
      views::CAPTION_BUTTON_ICON_MAXIMIZE_RESTORE, HTMAXBUTTON,
      views::kWindowControlMaximizeIcon,
      base::BindRepeating(&views::Widget::Maximize, base::Unretained(frame)));
  restore_button_ = CreateButton(
      VIEW_ID_RESTORE_BUTTON, IDS_ACCNAME_RESTORE,
      views::CAPTION_BUTTON_ICON_MAXIMIZE_RESTORE, HTMAXBUTTON,
      views::kWindowControlRestoreIcon,
      base::BindRepeating(&views::Widget::Restore, base::Unretained(frame)));
  close_button_ = CreateButton(
      VIEW_ID_CLOSE_BUTTON, IDS_ACCNAME_CLOSE, views::CAPTION_BUTTON_ICON_CLOSE,
      HTMAXBUTTON, views::kWindowControlCloseIcon,
      base::BindRepeating(&views::Widget::CloseWithReason,
                          base::Unretained(frame),
                          views::Widget::ClosedReason::kCloseButtonClicked));

  // Unretained() is safe because the subscription is saved into an instance
  // member and thus will be cancelled upon the instance's destruction.
  paint_as_active_changed_subscription_ =
      frame->RegisterPaintAsActiveChangedCallback(base::BindRepeating(
          &OpaqueFrameView::PaintAsActiveChanged, base::Unretained(this)));
}

int OpaqueFrameView::ResizingBorderHitTest(const gfx::Point& point) {
  return FramelessView::ResizingBorderHitTest(point);
}

void OpaqueFrameView::InvalidateCaptionButtons() {
  UpdateCaptionButtonPlaceholderContainerBackground();
  UpdateFrameCaptionButtons();
  LayoutWindowControlsOverlay();
  InvalidateLayout();
}

gfx::Rect OpaqueFrameView::GetBoundsForClientView() const {
  if (window()->IsWindowControlsOverlayEnabled()) {
    auto border_thickness = FrameBorderInsets(false);
    int top_height = border_thickness.top();
    return gfx::Rect(
        border_thickness.left(), top_height,
        std::max(0, width() - border_thickness.width()),
        std::max(0, height() - top_height - border_thickness.bottom()));
  }

  return FramelessView::GetBoundsForClientView();
}

gfx::Rect OpaqueFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  if (window()->IsWindowControlsOverlayEnabled()) {
    int top_height = NonClientTopHeight(false);
    auto border_insets = FrameBorderInsets(false);
    return gfx::Rect(
        std::max(0, client_bounds.x() - border_insets.left()),
        std::max(0, client_bounds.y() - top_height),
        client_bounds.width() + border_insets.width(),
        client_bounds.height() + top_height + border_insets.bottom());
  }

  return FramelessView::GetWindowBoundsForClientBounds(client_bounds);
}

int OpaqueFrameView::NonClientHitTest(const gfx::Point& point) {
  if (window()->IsWindowControlsOverlayEnabled()) {
    // Ensure support for resizing frameless window with border drag.
    int frame_component = ResizingBorderHitTest(point);
    if (frame_component != HTNOWHERE)
      return frame_component;

    if (HitTestCaptionButton(close_button_, point))
      return HTCLOSE;
    if (HitTestCaptionButton(restore_button_, point))
      return HTMAXBUTTON;
    if (HitTestCaptionButton(maximize_button_, point))
      return HTMAXBUTTON;
    if (HitTestCaptionButton(minimize_button_, point))
      return HTMINBUTTON;

    if (caption_button_placeholder_container_->GetMirroredBounds().Contains(
            point)) {
      return HTCAPTION;
    }
  }

  return FramelessView::NonClientHitTest(point);
}

void OpaqueFrameView::ResetWindowControls() {
  NonClientFrameView::ResetWindowControls();

  if (restore_button_)
    restore_button_->SetState(views::Button::STATE_NORMAL);
  if (minimize_button_)
    minimize_button_->SetState(views::Button::STATE_NORMAL);
  if (maximize_button_)
    maximize_button_->SetState(views::Button::STATE_NORMAL);
  // The close button isn't affected by this constraint.
}

views::View* OpaqueFrameView::TargetForRect(views::View* root,
                                            const gfx::Rect& rect) {
  return views::NonClientFrameView::TargetForRect(root, rect);
}

void OpaqueFrameView::Layout(PassKey) {
  LayoutSuperclass<FramelessView>(this);

  if (!window()->IsWindowControlsOverlayEnabled())
    return;

  // Reset all our data so that everything is invisible.
  TopAreaPadding top_area_padding = GetTopAreaPadding();
  available_space_leading_x_ = top_area_padding.leading;
  available_space_trailing_x_ = width() - top_area_padding.trailing;
  minimum_size_for_buttons_ =
      available_space_leading_x_ + width() - available_space_trailing_x_;
  placed_leading_button_ = false;
  placed_trailing_button_ = false;

  LayoutWindowControls();

  int height = NonClientTopHeight(false);
  auto insets = FrameBorderInsets(false);
  int container_x = placed_trailing_button_ ? available_space_trailing_x_ : 0;
  caption_button_placeholder_container_->SetBounds(
      container_x, insets.top(), minimum_size_for_buttons_ - insets.width(),
      height - insets.top());

  LayoutWindowControlsOverlay();
}

void OpaqueFrameView::OnPaint(gfx::Canvas* canvas) {
  if (!window()->IsWindowControlsOverlayEnabled())
    return;

  if (frame()->IsFullscreen())
    return;

  UpdateFrameCaptionButtons();
}

void OpaqueFrameView::PaintAsActiveChanged() {
  if (!window()->IsWindowControlsOverlayEnabled())
    return;

  UpdateCaptionButtonPlaceholderContainerBackground();
  UpdateFrameCaptionButtons();
}

void OpaqueFrameView::UpdateFrameCaptionButtons() {
  const bool active = ShouldPaintAsActive();
  const SkColor symbol_color = window()->overlay_symbol_color();
  const SkColor background_color = window()->overlay_button_color();
  SkColor frame_color =
      background_color == SkColor() ? GetFrameColor() : background_color;

  for (views::Button* button :
       {minimize_button_, maximize_button_, restore_button_, close_button_}) {
    DCHECK_EQ(std::string(views::FrameCaptionButton::kViewClassName),
              button->GetClassName());
    views::FrameCaptionButton* frame_caption_button =
        static_cast<views::FrameCaptionButton*>(button);
    frame_caption_button->SetPaintAsActive(active);
    frame_caption_button->SetButtonColor(symbol_color);
    frame_caption_button->SetBackgroundColor(frame_color);
  }
}

void OpaqueFrameView::UpdateCaptionButtonPlaceholderContainerBackground() {
  if (caption_button_placeholder_container_) {
    const SkColor obc = window()->overlay_button_color();
    const SkColor bg_color = obc == SkColor() ? GetFrameColor() : obc;
    caption_button_placeholder_container_->SetBackground(
        views::CreateSolidBackground(bg_color));
  }
}

void OpaqueFrameView::LayoutWindowControls() {
  // Keep a list of all buttons that we don't show.
  std::vector<views::FrameButton> buttons_not_shown;
  buttons_not_shown.push_back(views::FrameButton::kMaximize);
  buttons_not_shown.push_back(views::FrameButton::kMinimize);
  buttons_not_shown.push_back(views::FrameButton::kClose);

  for (const auto& button : leading_buttons_) {
    ConfigureButton(button, ALIGN_LEADING);
    std::erase(buttons_not_shown, button);
  }

  for (const auto& button : base::Reversed(trailing_buttons_)) {
    ConfigureButton(button, ALIGN_TRAILING);
    std::erase(buttons_not_shown, button);
  }

  for (const auto& button_id : buttons_not_shown)
    HideButton(button_id);
}

void OpaqueFrameView::LayoutWindowControlsOverlay() {
  int overlay_height = window()->titlebar_overlay_height();
  if (overlay_height == 0) {
    // Accounting for the 1 pixel margin at the top of the button container
    overlay_height =
        window()->IsMaximized()
            ? caption_button_placeholder_container_->size().height()
            : caption_button_placeholder_container_->size().height() + 1;
  }
  int overlay_width = caption_button_placeholder_container_->size().width();
  int bounding_rect_width = width() - overlay_width;
  auto bounding_rect =
      GetMirroredRect(gfx::Rect(0, 0, bounding_rect_width, overlay_height));

  window()->SetWindowControlsOverlayRect(bounding_rect);
  window()->NotifyLayoutWindowControlsOverlay();
}

views::Button* OpaqueFrameView::CreateButton(
    ViewID view_id,
    int accessibility_string_id,
    views::CaptionButtonIcon icon_type,
    int ht_component,
    const gfx::VectorIcon& icon_image,
    views::Button::PressedCallback callback) {
  views::FrameCaptionButton* button = new views::FrameCaptionButton(
      views::Button::PressedCallback(), icon_type, ht_component);
  button->SetImage(button->GetIcon(), views::FrameCaptionButton::Animate::kNo,
                   icon_image);

  button->SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
  button->SetCallback(std::move(callback));
  button->SetAccessibleName(l10n_util::GetStringUTF16(accessibility_string_id));
  button->SetID(view_id);
  AddChildView(button);

  button->SetPaintToLayer();
  button->layer()->SetFillsBoundsOpaquely(false);

  return button;
}

gfx::Insets OpaqueFrameView::FrameBorderInsets(bool restored) const {
  return !restored && IsFrameCondensed() ? gfx::Insets()
                                         : RestoredFrameBorderInsets();
}

int OpaqueFrameView::FrameTopBorderThickness(bool restored) const {
  int thickness = FrameBorderInsets(restored).top();
  if ((restored || !IsFrameCondensed()) && thickness > 0)
    thickness += NonClientExtraTopThickness();
  return thickness;
}

OpaqueFrameView::TopAreaPadding OpaqueFrameView::GetTopAreaPadding(
    bool has_leading_buttons,
    bool has_trailing_buttons) const {
  const auto padding = FrameBorderInsets(false);
  return TopAreaPadding{padding.left(), padding.right()};
}

bool OpaqueFrameView::IsFrameCondensed() const {
  return frame()->IsMaximized() || frame()->IsFullscreen();
}

gfx::Insets OpaqueFrameView::RestoredFrameBorderInsets() const {
  return gfx::Insets();
}

gfx::Insets OpaqueFrameView::RestoredFrameEdgeInsets() const {
  return gfx::Insets::TLBR(kTopFrameEdgeThickness, kSideFrameEdgeThickness,
                           kSideFrameEdgeThickness, kSideFrameEdgeThickness);
}

int OpaqueFrameView::NonClientExtraTopThickness() const {
  return kNonClientExtraTopThickness;
}

int OpaqueFrameView::NonClientTopHeight(bool restored) const {
  // Adding 2px of vertical padding puts at least 1 px of space on the top and
  // bottom of the element.
  constexpr int kVerticalPadding = 2;
  const int icon_height = GetIconSize() + kVerticalPadding;
  const int caption_button_height = DefaultCaptionButtonY(restored) +
                                    kCaptionButtonHeight +
                                    kCaptionButtonBottomPadding;

  int custom_height = window()->titlebar_overlay_height();
  return custom_height ? custom_height
                       : std::max(icon_height, caption_button_height) +
                             kContentEdgeShadowThickness;
}

int OpaqueFrameView::CaptionButtonY(views::FrameButton button_id,
                                    bool restored) const {
  return DefaultCaptionButtonY(restored);
}

int OpaqueFrameView::DefaultCaptionButtonY(bool restored) const {
  // Maximized buttons start at window top, since the window has no border. This
  // offset is for the image (the actual clickable bounds extend all the way to
  // the top to take Fitts' Law into account).
  const bool start_at_top_of_frame = !restored && IsFrameCondensed();
  return start_at_top_of_frame
             ? FrameBorderInsets(false).top()
             : views::NonClientFrameView::kFrameShadowThickness;
}

gfx::Insets OpaqueFrameView::FrameEdgeInsets(bool restored) const {
  return RestoredFrameEdgeInsets();
}

int OpaqueFrameView::GetIconSize() const {
  // The icon never shrinks below 16 px on a side.
  const int kIconMinimumSize = 16;
  return std::max(gfx::FontList().GetHeight(), kIconMinimumSize);
}

OpaqueFrameView::TopAreaPadding OpaqueFrameView::GetTopAreaPadding() const {
  return GetTopAreaPadding(!leading_buttons_.empty(),
                           !trailing_buttons_.empty());
}

SkColor OpaqueFrameView::GetFrameColor() const {
  return GetColorProvider()->GetColor(
      ShouldPaintAsActive() ? ui::kColorFrameActive : ui::kColorFrameInactive);
}

void OpaqueFrameView::ConfigureButton(views::FrameButton button_id,
                                      ButtonAlignment alignment) {
  switch (button_id) {
    case views::FrameButton::kMinimize: {
      bool can_minimize = true;  // delegate_->CanMinimize();
      if (can_minimize) {
        minimize_button_->SetVisible(true);
        SetBoundsForButton(button_id, minimize_button_, alignment);
      } else {
        HideButton(button_id);
      }
      break;
    }
    case views::FrameButton::kMaximize: {
      bool can_maximize = true;  // delegate_->CanMaximize();
      if (can_maximize) {
        // When the window is restored, we show a maximized button; otherwise,
        // we show a restore button.
        bool is_restored = !window()->IsMaximized() && !window()->IsMinimized();
        views::Button* invisible_button =
            is_restored ? restore_button_ : maximize_button_;
        invisible_button->SetVisible(false);

        views::Button* visible_button =
            is_restored ? maximize_button_ : restore_button_;
        visible_button->SetVisible(true);
        SetBoundsForButton(button_id, visible_button, alignment);
      } else {
        HideButton(button_id);
      }
      break;
    }
    case views::FrameButton::kClose: {
      close_button_->SetVisible(true);
      SetBoundsForButton(button_id, close_button_, alignment);
      break;
    }
  }
}

void OpaqueFrameView::HideButton(views::FrameButton button_id) {
  switch (button_id) {
    case views::FrameButton::kMinimize:
      minimize_button_->SetVisible(false);
      break;
    case views::FrameButton::kMaximize:
      restore_button_->SetVisible(false);
      maximize_button_->SetVisible(false);
      break;
    case views::FrameButton::kClose:
      close_button_->SetVisible(false);
      break;
  }
}

void OpaqueFrameView::SetBoundsForButton(views::FrameButton button_id,
                                         views::Button* button,
                                         ButtonAlignment alignment) {
  const int caption_y = CaptionButtonY(button_id, false);

  // There should always be the same number of non-shadow pixels visible to the
  // side of the caption buttons.  In maximized mode we extend buttons to the
  // screen top and the rightmost button to the screen right (or leftmost button
  // to the screen left, for left-aligned buttons) to obey Fitts' Law.
  const bool is_frame_condensed = IsFrameCondensed();

  const int button_width = views::GetCaptionButtonWidth();

  gfx::Size button_size = button->GetPreferredSize();
  DCHECK_EQ(std::string(views::FrameCaptionButton::kViewClassName),
            button->GetClassName());
  const int caption_button_center_size =
      button_width - 2 * views::kCaptionButtonInkDropDefaultCornerRadius;
  const int height = GetTopAreaHeight() - FrameEdgeInsets(false).top();
  const int corner_radius =
      std::clamp((height - caption_button_center_size) / 2, 0,
                 views::kCaptionButtonInkDropDefaultCornerRadius);
  button_size = gfx::Size(button_width, height);
  button->SetPreferredSize(button_size);
  static_cast<views::FrameCaptionButton*>(button)->SetInkDropCornerRadius(
      corner_radius);

  TopAreaPadding top_area_padding = GetTopAreaPadding();

  switch (alignment) {
    case ALIGN_LEADING: {
      int extra_width = top_area_padding.leading;
      int button_start_spacing =
          GetWindowCaptionSpacing(button_id, true, !placed_leading_button_);

      available_space_leading_x_ += button_start_spacing;
      minimum_size_for_buttons_ += button_start_spacing;

      bool top_spacing_clickable = is_frame_condensed;
      bool start_spacing_clickable =
          is_frame_condensed && !placed_leading_button_;
      button->SetBounds(
          available_space_leading_x_ - (start_spacing_clickable
                                            ? button_start_spacing + extra_width
                                            : 0),
          top_spacing_clickable ? 0 : caption_y,
          button_size.width() + (start_spacing_clickable
                                     ? button_start_spacing + extra_width
                                     : 0),
          button_size.height() + (top_spacing_clickable ? caption_y : 0));

      int button_end_spacing =
          GetWindowCaptionSpacing(button_id, false, !placed_leading_button_);
      available_space_leading_x_ += button_size.width() + button_end_spacing;
      minimum_size_for_buttons_ += button_size.width() + button_end_spacing;
      placed_leading_button_ = true;
      break;
    }
    case ALIGN_TRAILING: {
      int extra_width = top_area_padding.trailing;
      int button_start_spacing =
          GetWindowCaptionSpacing(button_id, true, !placed_trailing_button_);

      available_space_trailing_x_ -= button_start_spacing;
      minimum_size_for_buttons_ += button_start_spacing;

      bool top_spacing_clickable = is_frame_condensed;
      bool start_spacing_clickable =
          is_frame_condensed && !placed_trailing_button_;
      button->SetBounds(
          available_space_trailing_x_ - button_size.width(),
          top_spacing_clickable ? 0 : caption_y,
          button_size.width() + (start_spacing_clickable
                                     ? button_start_spacing + extra_width
                                     : 0),
          button_size.height() + (top_spacing_clickable ? caption_y : 0));

      int button_end_spacing =
          GetWindowCaptionSpacing(button_id, false, !placed_trailing_button_);
      available_space_trailing_x_ -= button_size.width() + button_end_spacing;
      minimum_size_for_buttons_ += button_size.width() + button_end_spacing;
      placed_trailing_button_ = true;
      break;
    }
  }
}

int OpaqueFrameView::GetTopAreaHeight() const {
  int top_height = NonClientTopHeight(false);
  return top_height;
}

int OpaqueFrameView::GetWindowCaptionSpacing(views::FrameButton button_id,
                                             bool leading_spacing,
                                             bool is_leading_button) const {
  return 0;
}

BEGIN_METADATA(OpaqueFrameView)
END_METADATA

}  // namespace electron
