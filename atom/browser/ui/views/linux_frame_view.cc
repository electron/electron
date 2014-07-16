// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/linux_frame_view.h"

#include <algorithm>

#include "atom/browser/native_window_views.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/hit_test.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/path.h"
#include "ui/views/color_constants.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/views_delegate.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/window/client_view.h"
#include "ui/views/window/frame_background.h"
#include "ui/views/window/window_resources.h"
#include "ui/views/window/window_shape.h"

// Workaround for including grit headers.
#include "ui/ui_resources/grit/ui_resources.h"
#include "ui/ui_strings/grit/ui_strings.h"

namespace atom {

namespace {

// The frame border is only visible in restored mode and is hardcoded to 4 px on
// each side regardless of the system window border size.
const int kFrameBorderThickness = 4;
// In the window corners, the resize areas don't actually expand bigger, but the
// 16 px at the end of each edge triggers diagonal resizing.
const int kResizeAreaCornerSize = 16;
// The titlebar never shrinks too short to show the caption button plus some
// padding below it.
const int kCaptionButtonHeightWithPadding = 19;
// The titlebar has a 2 px 3D edge along the top and bottom.
const int kTitlebarTopAndBottomEdgeThickness = 2;
// The icon is inset 2 px from the left frame border.
const int kIconLeftSpacing = 2;
// The icon never shrinks below 16 px on a side.
const int kIconMinimumSize = 16;
// The space between the window icon and the title text.
const int kTitleIconOffsetX = 4;
// The space between the title text and the caption buttons.
const int kTitleCaptionSpacing = 5;

#if defined(OS_CHROMEOS)
// Chrome OS uses a dark gray.
const SkColor kDefaultColorFrame = SkColorSetRGB(109, 109, 109);
const SkColor kDefaultColorFrameInactive = SkColorSetRGB(176, 176, 176);
#else
// Windows and Linux use a blue.
const SkColor kDefaultColorFrame = SkColorSetRGB(66, 116, 201);
const SkColor kDefaultColorFrameInactive = SkColorSetRGB(161, 182, 228);
#endif

const gfx::FontList& GetTitleFontList() {
  static const gfx::FontList title_font_list =
      views::NativeWidgetAura::GetWindowTitleFontList();
  return title_font_list;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// LinuxFrameView, public:

LinuxFrameView::LinuxFrameView()
    : window_icon_(NULL),
      minimize_button_(NULL),
      maximize_button_(NULL),
      restore_button_(NULL),
      close_button_(NULL),
      should_show_maximize_button_(false),
      frame_background_(new views::FrameBackground) {
}

LinuxFrameView::~LinuxFrameView() {
}

void LinuxFrameView::Init(NativeWindowViews* window, views::Widget* frame) {
  FramelessView::Init(window, frame);

  close_button_ = new views::ImageButton(this);
  close_button_->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_APP_ACCNAME_CLOSE));

  // Close button images will be set in LayoutWindowControls().
  AddChildView(close_button_);

  minimize_button_ = InitWindowCaptionButton(IDS_APP_ACCNAME_MINIMIZE,
      IDR_MINIMIZE, IDR_MINIMIZE_H, IDR_MINIMIZE_P);

  maximize_button_ = InitWindowCaptionButton(IDS_APP_ACCNAME_MAXIMIZE,
      IDR_MAXIMIZE, IDR_MAXIMIZE_H, IDR_MAXIMIZE_P);

  restore_button_ = InitWindowCaptionButton(IDS_APP_ACCNAME_RESTORE,
      IDR_RESTORE, IDR_RESTORE_H, IDR_RESTORE_P);

  should_show_maximize_button_ = frame_->widget_delegate()->CanMaximize();

  if (frame_->widget_delegate()->ShouldShowWindowIcon()) {
    window_icon_ = new views::ImageButton(this);
    AddChildView(window_icon_);
  }
}

///////////////////////////////////////////////////////////////////////////////
// LinuxFrameView, NonClientFrameView implementation:

gfx::Rect LinuxFrameView::GetBoundsForClientView() const {
  return client_view_bounds_;
}

gfx::Rect LinuxFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  int top_height = NonClientTopBorderHeight();
  int border_thickness = NonClientBorderThickness();
  return gfx::Rect(client_bounds.x() - border_thickness,
                   client_bounds.y() - top_height,
                   client_bounds.width() + (2 * border_thickness),
                   client_bounds.height() + top_height + border_thickness);
}

int LinuxFrameView::NonClientHitTest(const gfx::Point& point) {
  if (!window_->has_frame())
    return FramelessView::NonClientHitTest(point);

  // Sanity check.
  if (!bounds().Contains(point))
    return HTNOWHERE;

  int frame_component = frame_->client_view()->NonClientHitTest(point);

  // See if we're in the sysmenu region.  (We check the ClientView first to be
  // consistent with OpaqueBrowserFrameView; it's not really necessary here.)
  gfx::Rect sysmenu_rect(IconBounds());
  // In maximized mode we extend the rect to the screen corner to take advantage
  // of Fitts' Law.
  if (frame_->IsMaximized())
    sysmenu_rect.SetRect(0, 0, sysmenu_rect.right(), sysmenu_rect.bottom());
  sysmenu_rect.set_x(GetMirroredXForRect(sysmenu_rect));
  if (sysmenu_rect.Contains(point))
    return (frame_component == HTCLIENT) ? HTCLIENT : HTSYSMENU;

  if (frame_component != HTNOWHERE)
    return frame_component;

  // Then see if the point is within any of the window controls.
  if (close_button_->GetMirroredBounds().Contains(point))
    return HTCLOSE;
  if (restore_button_->GetMirroredBounds().Contains(point))
    return HTMAXBUTTON;
  if (maximize_button_->GetMirroredBounds().Contains(point))
    return HTMAXBUTTON;
  if (minimize_button_->GetMirroredBounds().Contains(point))
    return HTMINBUTTON;
  if (window_icon_ && window_icon_->GetMirroredBounds().Contains(point))
    return HTSYSMENU;

  int window_component = GetHTComponentForFrame(point, FrameBorderThickness(),
      NonClientBorderThickness(), kResizeAreaCornerSize, kResizeAreaCornerSize,
      frame_->widget_delegate()->CanResize());
  // Fall back to the caption if no other component matches.
  return (window_component == HTNOWHERE) ? HTCAPTION : window_component;
}

void LinuxFrameView::GetWindowMask(const gfx::Size& size,
                                    gfx::Path* window_mask) {
  DCHECK(window_mask);
  if (frame_->IsMaximized() || !ShouldShowTitleBarAndBorder())
    return;

  views::GetDefaultWindowMask(size, window_mask);
}

void LinuxFrameView::ResetWindowControls() {
  restore_button_->SetState(views::CustomButton::STATE_NORMAL);
  minimize_button_->SetState(views::CustomButton::STATE_NORMAL);
  maximize_button_->SetState(views::CustomButton::STATE_NORMAL);
  // The close button isn't affected by this constraint.
}

void LinuxFrameView::UpdateWindowIcon() {
  if (window_icon_)
    window_icon_->SchedulePaint();
}

void LinuxFrameView::UpdateWindowTitle() {
  SchedulePaintInRect(title_bounds_);
}

///////////////////////////////////////////////////////////////////////////////
// LinuxFrameView, View overrides:

void LinuxFrameView::OnPaint(gfx::Canvas* canvas) {
  if (!ShouldShowTitleBarAndBorder())
    return;

  if (frame_->IsMaximized())
    PaintMaximizedFrameBorder(canvas);
  else
    PaintRestoredFrameBorder(canvas);
  PaintTitleBar(canvas);
  if (ShouldShowClientEdge())
    PaintRestoredClientEdge(canvas);
}

void LinuxFrameView::Layout() {
  if (ShouldShowTitleBarAndBorder()) {
    LayoutWindowControls();
    LayoutTitleBar();
  }

  LayoutClientView();
}

gfx::Size LinuxFrameView::GetPreferredSize() {
  return frame_->non_client_view()->GetWindowBoundsForClientBounds(
      gfx::Rect(frame_->client_view()->GetPreferredSize())).size();
}

gfx::Size LinuxFrameView::GetMinimumSize() {
  return frame_->non_client_view()->GetWindowBoundsForClientBounds(
      gfx::Rect(frame_->client_view()->GetMinimumSize())).size();
}

gfx::Size LinuxFrameView::GetMaximumSize() {
  gfx::Size max_size = frame_->client_view()->GetMaximumSize();
  gfx::Size converted_size =
      frame_->non_client_view()->GetWindowBoundsForClientBounds(
          gfx::Rect(max_size)).size();
  return gfx::Size(max_size.width() == 0 ? 0 : converted_size.width(),
                   max_size.height() == 0 ? 0 : converted_size.height());
}

///////////////////////////////////////////////////////////////////////////////
// LinuxFrameView, ButtonListener implementation:

void LinuxFrameView::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  if (sender == close_button_)
    frame_->Close();
  else if (sender == minimize_button_)
    frame_->Minimize();
  else if (sender == maximize_button_)
    frame_->Maximize();
  else if (sender == restore_button_)
    frame_->Restore();
}

///////////////////////////////////////////////////////////////////////////////
// LinuxFrameView, private:

int LinuxFrameView::FrameBorderThickness() const {
  return frame_->IsMaximized() ? 0 : kFrameBorderThickness;
}

int LinuxFrameView::NonClientBorderThickness() const {
  // In maximized mode, we don't show a client edge.
  return FrameBorderThickness() +
      (ShouldShowClientEdge() ? kClientEdgeThickness : 0);
}

int LinuxFrameView::NonClientTopBorderHeight() const {
  return std::max(FrameBorderThickness() + IconSize(),
                  CaptionButtonY() + kCaptionButtonHeightWithPadding) +
      TitlebarBottomThickness();
}

int LinuxFrameView::CaptionButtonY() const {
  // Maximized buttons start at window top so that even if their images aren't
  // drawn flush with the screen edge, they still obey Fitts' Law.
  return frame_->IsMaximized() ? FrameBorderThickness() : kFrameShadowThickness;
}

int LinuxFrameView::TitlebarBottomThickness() const {
  return kTitlebarTopAndBottomEdgeThickness +
      (ShouldShowClientEdge() ? kClientEdgeThickness : 0);
}

int LinuxFrameView::IconSize() const {
#if defined(OS_WIN)
  // This metric scales up if either the titlebar height or the titlebar font
  // size are increased.
  return GetSystemMetrics(SM_CYSMICON);
#else
  return std::max(GetTitleFontList().GetHeight(), kIconMinimumSize);
#endif
}

gfx::Rect LinuxFrameView::IconBounds() const {
  int size = IconSize();
  int frame_thickness = FrameBorderThickness();
  // Our frame border has a different "3D look" than Windows'.  Theirs has a
  // more complex gradient on the top that they push their icon/title below;
  // then the maximized window cuts this off and the icon/title are centered
  // in the remaining space.  Because the apparent shape of our border is
  // simpler, using the same positioning makes things look slightly uncentered
  // with restored windows, so when the window is restored, instead of
  // calculating the remaining space from below the frame border, we calculate
  // from below the 3D edge.
  int unavailable_px_at_top = frame_->IsMaximized() ?
      frame_thickness : kTitlebarTopAndBottomEdgeThickness;
  // When the icon is shorter than the minimum space we reserve for the caption
  // button, we vertically center it.  We want to bias rounding to put extra
  // space above the icon, since the 3D edge (+ client edge, for restored
  // windows) below looks (to the eye) more like additional space than does the
  // 3D edge (or nothing at all, for maximized windows) above; hence the +1.
  int y = unavailable_px_at_top + (NonClientTopBorderHeight() -
      unavailable_px_at_top - size - TitlebarBottomThickness() + 1) / 2;
  return gfx::Rect(frame_thickness + kIconLeftSpacing, y, size, size);
}

bool LinuxFrameView::ShouldShowTitleBarAndBorder() const {
  if (!window_->has_frame())
    return false;

  if (frame_->IsFullscreen())
    return false;

  if (views::ViewsDelegate::views_delegate) {
    return !views::ViewsDelegate::views_delegate->WindowManagerProvidesTitleBar(
        frame_->IsMaximized());
  }

  return true;
}

bool LinuxFrameView::ShouldShowClientEdge() const {
  return !frame_->IsMaximized() && ShouldShowTitleBarAndBorder();
}

void LinuxFrameView::PaintRestoredFrameBorder(gfx::Canvas* canvas) {
  frame_background_->set_frame_color(GetFrameColor());
  const gfx::ImageSkia* frame_image = GetFrameImage();
  frame_background_->set_theme_image(frame_image);
  frame_background_->set_top_area_height(frame_image->height());

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();

  frame_background_->SetCornerImages(
      rb.GetImageNamed(IDR_WINDOW_TOP_LEFT_CORNER).ToImageSkia(),
      rb.GetImageNamed(IDR_WINDOW_TOP_RIGHT_CORNER).ToImageSkia(),
      rb.GetImageNamed(IDR_WINDOW_BOTTOM_LEFT_CORNER).ToImageSkia(),
      rb.GetImageNamed(IDR_WINDOW_BOTTOM_RIGHT_CORNER).ToImageSkia());
  frame_background_->SetSideImages(
      rb.GetImageNamed(IDR_WINDOW_LEFT_SIDE).ToImageSkia(),
      rb.GetImageNamed(IDR_WINDOW_TOP_CENTER).ToImageSkia(),
      rb.GetImageNamed(IDR_WINDOW_RIGHT_SIDE).ToImageSkia(),
      rb.GetImageNamed(IDR_WINDOW_BOTTOM_CENTER).ToImageSkia());

  frame_background_->PaintRestored(canvas, this);
}

void LinuxFrameView::PaintMaximizedFrameBorder(gfx::Canvas* canvas) {
  const gfx::ImageSkia* frame_image = GetFrameImage();
  frame_background_->set_theme_image(frame_image);
  frame_background_->set_top_area_height(frame_image->height());
  frame_background_->PaintMaximized(canvas, this);

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();

  // TODO(jamescook): Migrate this into FrameBackground.
  // The bottom of the titlebar actually comes from the top of the Client Edge
  // graphic, with the actual client edge clipped off the bottom.
  const gfx::ImageSkia* titlebar_bottom = rb.GetImageNamed(
      IDR_APP_TOP_CENTER).ToImageSkia();
  int edge_height = titlebar_bottom->height() -
      (ShouldShowClientEdge() ? kClientEdgeThickness : 0);
  canvas->TileImageInt(*titlebar_bottom, 0,
      frame_->client_view()->y() - edge_height, width(), edge_height);
}

void LinuxFrameView::PaintTitleBar(gfx::Canvas* canvas) {
  views::WidgetDelegate* delegate = frame_->widget_delegate();

  // It seems like in some conditions we can be asked to paint after the window
  // that contains us is WM_DESTROYed. At this point, our delegate is NULL. The
  // correct long term fix may be to shut down the RootView in WM_DESTROY.
  if (!delegate)
    return;

  gfx::Rect rect = title_bounds_;
  rect.set_x(GetMirroredXForRect(title_bounds_));
  canvas->DrawStringRect(delegate->GetWindowTitle(), GetTitleFontList(),
                         SK_ColorWHITE, rect);
}

void LinuxFrameView::PaintRestoredClientEdge(gfx::Canvas* canvas) {
  gfx::Rect client_area_bounds = frame_->client_view()->bounds();
  int client_area_top = client_area_bounds.y();

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();

  // Top: left, center, right sides.
  const gfx::ImageSkia* top_left = rb.GetImageSkiaNamed(IDR_APP_TOP_LEFT);
  const gfx::ImageSkia* top_center = rb.GetImageSkiaNamed(IDR_APP_TOP_CENTER);
  const gfx::ImageSkia* top_right = rb.GetImageSkiaNamed(IDR_APP_TOP_RIGHT);
  int top_edge_y = client_area_top - top_center->height();
  canvas->DrawImageInt(*top_left,
                       client_area_bounds.x() - top_left->width(),
                       top_edge_y);
  canvas->TileImageInt(*top_center,
                       client_area_bounds.x(),
                       top_edge_y,
                       client_area_bounds.width(),
                       top_center->height());
  canvas->DrawImageInt(*top_right, client_area_bounds.right(), top_edge_y);

  // Right side.
  const gfx::ImageSkia* right = rb.GetImageSkiaNamed(IDR_CONTENT_RIGHT_SIDE);
  int client_area_bottom =
      std::max(client_area_top, client_area_bounds.bottom());
  int client_area_height = client_area_bottom - client_area_top;
  canvas->TileImageInt(*right,
                       client_area_bounds.right(),
                       client_area_top,
                       right->width(),
                       client_area_height);

  // Bottom: left, center, right sides.
  const gfx::ImageSkia* bottom_left =
      rb.GetImageSkiaNamed(IDR_CONTENT_BOTTOM_LEFT_CORNER);
  const gfx::ImageSkia* bottom_center =
      rb.GetImageSkiaNamed(IDR_CONTENT_BOTTOM_CENTER);
  const gfx::ImageSkia* bottom_right =
      rb.GetImageSkiaNamed(IDR_CONTENT_BOTTOM_RIGHT_CORNER);

  canvas->DrawImageInt(*bottom_left,
                       client_area_bounds.x() - bottom_left->width(),
                       client_area_bottom);

  canvas->TileImageInt(*bottom_center,
                       client_area_bounds.x(),
                       client_area_bottom,
                       client_area_bounds.width(),
                       bottom_right->height());

  canvas->DrawImageInt(*bottom_right,
                       client_area_bounds.right(),
                       client_area_bottom);
  // Left side.
  const gfx::ImageSkia* left = rb.GetImageSkiaNamed(IDR_CONTENT_LEFT_SIDE);
  canvas->TileImageInt(*left,
                       client_area_bounds.x() - left->width(),
                       client_area_top,
                       left->width(),
                       client_area_height);

  // Draw the color to fill in the edges.
  canvas->FillRect(gfx::Rect(client_area_bounds.x() - 1,
                             client_area_top - 1,
                             client_area_bounds.width() + 1,
                             client_area_bottom - client_area_top + 1),
                   views::kClientEdgeColor);
}

SkColor LinuxFrameView::GetFrameColor() const {
  return frame_->IsActive() ? kDefaultColorFrame : kDefaultColorFrameInactive;
}

const gfx::ImageSkia* LinuxFrameView::GetFrameImage() const {
  return ui::ResourceBundle::GetSharedInstance().GetImageNamed(
      frame_->IsActive() ? IDR_FRAME : IDR_FRAME_INACTIVE).ToImageSkia();
}

void LinuxFrameView::LayoutWindowControls() {
  close_button_->SetImageAlignment(views::ImageButton::ALIGN_LEFT,
                                   views::ImageButton::ALIGN_BOTTOM);
  int caption_y = CaptionButtonY();
  bool is_maximized = frame_->IsMaximized();
  // There should always be the same number of non-shadow pixels visible to the
  // side of the caption buttons.  In maximized mode we extend the rightmost
  // button to the screen corner to obey Fitts' Law.
  int right_extra_width = is_maximized ?
      (kFrameBorderThickness - kFrameShadowThickness) : 0;
  gfx::Size close_button_size = close_button_->GetPreferredSize();
  close_button_->SetBounds(width() - FrameBorderThickness() -
      right_extra_width - close_button_size.width(), caption_y,
      close_button_size.width() + right_extra_width,
      close_button_size.height());

  // When the window is restored, we show a maximized button; otherwise, we show
  // a restore button.
  bool is_restored = !is_maximized && !frame_->IsMinimized();
  views::ImageButton* invisible_button = is_restored ? restore_button_
                                                     : maximize_button_;
  invisible_button->SetVisible(false);

  views::ImageButton* visible_button = is_restored ? maximize_button_
                                                   : restore_button_;
  views::FramePartImage normal_part, hot_part, pushed_part;
  int next_button_x;
  if (should_show_maximize_button_) {
    visible_button->SetVisible(true);
    visible_button->SetImageAlignment(views::ImageButton::ALIGN_LEFT,
                                      views::ImageButton::ALIGN_BOTTOM);
    gfx::Size visible_button_size = visible_button->GetPreferredSize();
    visible_button->SetBounds(close_button_->x() - visible_button_size.width(),
                              caption_y, visible_button_size.width(),
                              visible_button_size.height());
    next_button_x = visible_button->x();
  } else {
    visible_button->SetVisible(false);
    next_button_x = close_button_->x();
  }

  minimize_button_->SetVisible(true);
  minimize_button_->SetImageAlignment(views::ImageButton::ALIGN_LEFT,
                                      views::ImageButton::ALIGN_BOTTOM);
  gfx::Size minimize_button_size = minimize_button_->GetPreferredSize();
  minimize_button_->SetBounds(
      next_button_x - minimize_button_size.width(), caption_y,
      minimize_button_size.width(),
      minimize_button_size.height());

  normal_part = IDR_CLOSE;
  hot_part = IDR_CLOSE_H;
  pushed_part = IDR_CLOSE_P;

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();

  close_button_->SetImage(views::CustomButton::STATE_NORMAL,
                          rb.GetImageNamed(normal_part).ToImageSkia());
  close_button_->SetImage(views::CustomButton::STATE_HOVERED,
                          rb.GetImageNamed(hot_part).ToImageSkia());
  close_button_->SetImage(views::CustomButton::STATE_PRESSED,
                          rb.GetImageNamed(pushed_part).ToImageSkia());
}

void LinuxFrameView::LayoutTitleBar() {
  // The window title position is calculated based on the icon position, even
  // when there is no icon.
  gfx::Rect icon_bounds(IconBounds());
  bool show_window_icon = window_icon_ != NULL;
  if (show_window_icon)
    window_icon_->SetBoundsRect(icon_bounds);

  // The offset between the window left edge and the title text.
  int title_x = show_window_icon ? icon_bounds.right() + kTitleIconOffsetX
                                 : icon_bounds.x();
  int title_height = GetTitleFontList().GetHeight();
  // We bias the title position so that when the difference between the icon and
  // title heights is odd, the extra pixel of the title is above the vertical
  // midline rather than below.  This compensates for how the icon is already
  // biased downwards (see IconBounds()) and helps prevent descenders on the
  // title from overlapping the 3D edge at the bottom of the titlebar.
  title_bounds_.SetRect(title_x,
      icon_bounds.y() + ((icon_bounds.height() - title_height - 1) / 2),
      std::max(0, minimize_button_->x() - kTitleCaptionSpacing -
      title_x), title_height);
}

void LinuxFrameView::LayoutClientView() {
  if (!ShouldShowTitleBarAndBorder()) {
    client_view_bounds_ = bounds();
    return;
  }

  int top_height = NonClientTopBorderHeight();
  int border_thickness = NonClientBorderThickness();
  client_view_bounds_.SetRect(border_thickness, top_height,
      std::max(0, width() - (2 * border_thickness)),
      std::max(0, height() - top_height - border_thickness));
}

views::ImageButton* LinuxFrameView::InitWindowCaptionButton(
    int accessibility_string_id,
    int normal_image_id,
    int hot_image_id,
    int pushed_image_id) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  views::ImageButton* button = new views::ImageButton(this);
  button->SetAccessibleName(l10n_util::GetStringUTF16(accessibility_string_id));
  button->SetImage(views::CustomButton::STATE_NORMAL,
                   rb.GetImageNamed(normal_image_id).ToImageSkia());
  button->SetImage(views::CustomButton::STATE_HOVERED,
                   rb.GetImageNamed(hot_image_id).ToImageSkia());
  button->SetImage(views::CustomButton::STATE_PRESSED,
                   rb.GetImageNamed(pushed_image_id).ToImageSkia());
  AddChildView(button);
  return button;
}

}  // namespace atom
