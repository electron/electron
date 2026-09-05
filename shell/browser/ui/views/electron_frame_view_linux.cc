// Copyright (c) 2026 Athul Iddya.
// Copyright (c) 2024 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/electron_frame_view_linux.h"

#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/i18n/rtl.h"
#include "base/time/time.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/ui/views/caption_button_placeholder_container.h"
#include "shell/browser/ui/views/electron_frame_view_layout_linux.h"
#include "shell/browser/ui/views/freedesktop_nav_button_provider.h"
#include "ui/base/hit_test.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/frame_background.h"
#include "ui/views/window/frame_caption_button.h"

namespace electron {

namespace {

// Values used for internal resizing when there are no insets.
const int kResizeAreaCornerSize = 16;
const int kResizeInsideBoundsSize = 5;

}  // namespace

ElectronFrameViewLinux::ElectronFrameViewLinux(
    NativeWindowViews* window,
    views::Widget* widget,
    std::unique_ptr<ui::NavButtonProvider> nav_button_provider,
    ElectronFrameViewLayoutLinux* layout,
    FreedesktopNavButtonProvider* freedesktop_provider)
    // The frame provider getter is unused when a layout is supplied; the
    // layout carries its own null-resolving getter.
    : NativeFrameViewLinux(widget,
                           std::move(nav_button_provider),
                           base::NullCallback(),
                           layout),
      window_(window),
      freedesktop_provider_(freedesktop_provider) {
  InitViews();
}

ElectronFrameViewLinux::~ElectronFrameViewLinux() {
  // The layout manager is owned by View and outlives this destructor; clear
  // its reference to the provider we own before the provider is destroyed.
  efv_layout()->set_nav_buttons(nullptr);
}

ElectronFrameViewLayoutLinux* ElectronFrameViewLinux::efv_layout() const {
  return static_cast<ElectronFrameViewLayoutLinux*>(layout());
}

bool ElectronFrameViewLinux::WantsFrame() const {
  return efv_layout()->wants_frame();
}

void ElectronFrameViewLinux::SetWantsFrame(bool wants_frame) {
  efv_layout()->set_wants_frame(wants_frame);
  InvalidateLayout();
  SchedulePaint();
}

void ElectronFrameViewLinux::CreateCaptionButtons() {
  if (!window_->IsWindowControlsOverlayEnabled())
    return;

  leading_button_container_ =
      AddChildView(std::make_unique<CaptionButtonPlaceholderContainer>());
  trailing_button_container_ =
      AddChildView(std::make_unique<CaptionButtonPlaceholderContainer>());

  // Allow containers to be clipped for rounded corners.
  for (auto* container :
       {leading_button_container_.get(), trailing_button_container_.get()}) {
    container->layer()->SetFillsBoundsOpaquely(false);
    container->layer()->SetIsFastRoundedCorner(true);
  }

  // ImageButtons with provider-rendered images, or vector caption buttons
  // when no provider is available.
  if (efv_layout()->nav_buttons())
    NativeFrameViewLinux::CreateCaptionButtons();
  else
    FrameViewLinux::CreateCaptionButtons();

  // Promote to layers so buttons render above the overlapping client view.
  for (auto* btn : {minimize_button(), maximize_button(), restore_button(),
                    close_button()}) {
    if (btn) {
      btn->SetPaintToLayer();
      btn->layer()->SetFillsBoundsOpaquely(false);
      // Disable the hover cross-fade: ImageButton blends the state images
      // through an ImageSkia that rasterizes at the nearest integer scale,
      // which momentarily shifts/blurs the glyph on fractional-scale
      // displays. Native titlebuttons switch state instantly anyway.
      btn->SetAnimationDuration(base::TimeDelta());
    }
  }
}

bool ElectronFrameViewLinux::HasWindowTitle() const {
  // Electron windows using this frame view are frameless; never paint a title.
  return false;
}

int ElectronFrameViewLinux::NonClientHitTest(const gfx::Point& point) {
  int result = FrameViewLinux::NonClientHitTest(point);
  if (result != HTCLIENT)
    return result;

  // The base class returns HTCLIENT for the WCO titlebar area because the
  // client view overlaps it. Check buttons and the draggable overlay region.
  if (window_->IsWindowControlsOverlayEnabled()) {
    using Type = ui::NavButtonProvider::FrameButtonDisplayType;
    auto check_button = [&](Type type, int hit_code) -> int {
      auto* button = GetButtonFromType(type);
      if (button && button->GetVisible() &&
          button->GetMirroredBounds().Contains(point))
        return hit_code;
      return HTNOWHERE;
    };

    result = check_button(Type::kClose, HTCLOSE);
    if (result != HTNOWHERE)
      return result;

    result = check_button(Type::kRestore, HTMAXBUTTON);
    if (result != HTNOWHERE)
      return result;

    result = check_button(Type::kMaximize, HTMAXBUTTON);
    if (result != HTNOWHERE)
      return result;

    result = check_button(Type::kMinimize, HTMINBUTTON);
    if (result != HTNOWHERE)
      return result;

    if (efv_layout()->GetLeadingButtonRect().Contains(point) ||
        efv_layout()->GetTrailingButtonRect().Contains(point))
      return HTCAPTION;
  }

  int contents_hit_test = window_->NonClientHitTest(point);
  if (contents_hit_test != HTNOWHERE)
    return contents_hit_test;

  return HTCLIENT;
}

int ElectronFrameViewLinux::ResizingBorderHitTest(const gfx::Point& point) {
  // Fall back to an internal resize band only when there are no frame insets
  // but the window is still supposed to be resizable. The main use case is
  // transparent windows. Matches the behavior of
  // WinFrameView::ResizingBorderHitTest.
  if (!window_->IsResizable() ||
      !efv_layout()->GetRestoredFrameBorderInsets().IsEmpty() ||
      GetWidget()->IsMaximized() || GetWidget()->IsFullscreen())
    return HTNOWHERE;

  return GetHTComponentForFrame(point, gfx::Insets(kResizeInsideBoundsSize),
                                kResizeAreaCornerSize, kResizeAreaCornerSize,
                                /*can_resize=*/true);
}

void ElectronFrameViewLinux::Layout(PassKey) {
  // Route through NativeFrameViewLinux::Layout only when provider-rendered
  // buttons exist: it refreshes the cached button images, which requires
  // both a provider and buttons to apply them to.
  if (efv_layout()->nav_buttons() && close_button())
    LayoutSuperclass<NativeFrameViewLinux>(this);
  else
    LayoutSuperclass<FrameViewLinux>(this);

  if (auto* iwcv = window_->primary_web_contents_view())
    iwcv->SetCornerRadii(GetCornerRadii());

  if (!window_->IsWindowControlsOverlayEnabled())
    return;

  // Hide WCO caption buttons in fullscreen.
  if (GetWidget()->IsFullscreen()) {
    for (auto* btn : {minimize_button(), maximize_button(), restore_button(),
                      close_button()}) {
      if (btn)
        btn->SetVisible(false);
    }
  }

  leading_button_container_->SetBoundsRect(
      efv_layout()->GetLeadingButtonRect());
  trailing_button_container_->SetBoundsRect(
      efv_layout()->GetTrailingButtonRect());

  // Apply the frame's corner radius to the button containers.
  const float radius = GetCornerRadii().upper_left();
  const bool rtl = base::i18n::IsRTL();
  leading_button_container_->layer()->SetRoundedCornerRadius(
      rtl ? gfx::RoundedCornersF(0, radius, 0, 0)
          : gfx::RoundedCornersF(radius, 0, 0, 0));
  trailing_button_container_->layer()->SetRoundedCornerRadius(
      rtl ? gfx::RoundedCornersF(radius, 0, 0, 0)
          : gfx::RoundedCornersF(0, radius, 0, 0));

  UpdateCaptionButtonPlaceholderContainerBackground();
  LayoutWindowControlsOverlay();
}

void ElectronFrameViewLinux::PaintRestoredFrameBorder(gfx::Canvas* canvas) {
  if (!WantsFrame())
    return;

  // Prevent the default system titlebar background from being drawn at the top
  // of frameless windows. Does not affect WCO which draws its own top area.
  frame_background()->set_top_area_height(0);

  FrameViewLinux::PaintRestoredFrameBorder(canvas);
}

void ElectronFrameViewLinux::PaintMaximizedFrameBorder(gfx::Canvas* canvas) {
  // No-op: WCO has a non-zero top area and the base class would otherwise
  // paint a full-width frame-color bar across it.
}

gfx::Size ElectronFrameViewLinux::GetMinimumSize() const {
  return window_->GetMinimumSize();
}

gfx::Size ElectronFrameViewLinux::GetMaximumSize() const {
  return window_->GetMaximumSize();
}

void ElectronFrameViewLinux::UpdateButtonColors() {
  if (freedesktop_provider_) {
    // Feed the effective titlebar background — and the app's symbolColor,
    // if set — to the provider. The glyph takes the symbol color and each
    // style's hover treatment derives from it; without one, colors derive
    // from the backdrop for legibility.
    const auto* color_provider = GetColorProvider();
    if (!color_provider)
      return;
    const bool active = ShouldPaintAsActive();
    const SkColor background = SkColorSetA(
        window_->overlay_button_color().value_or(color_provider->GetColor(
            active ? ui::kColorFrameActive : ui::kColorFrameInactive)),
        SK_AlphaOPAQUE);
    const std::optional<SkColor> symbol = window_->overlay_symbol_color();
    if (last_titlebar_background_ != background ||
        last_symbol_color_ != symbol) {
      last_titlebar_background_ = background;
      last_symbol_color_ = symbol;
      freedesktop_provider_->SetTitlebarColors(background, symbol);
      // Drop the cached button images so they re-render with the new colors
      // on the next layout.
      OnThemeOrButtonOrderChanged();
    }
    return;
  }
  // Other provider-rendered buttons (GTK-rasterized) carry their own colors.
  if (efv_layout()->nav_buttons())
    return;
  if (!window_->IsWindowControlsOverlayEnabled())
    return;

  // Apply custom WCO overlay colors if set.
  const bool active = ShouldPaintAsActive();
  const SkColor symbol_color =
      window_->overlay_symbol_color().value_or(SkColor());
  const std::optional<SkColor> background_color =
      window_->overlay_button_color();
  // Frame color is used for blending, force it to be opaque
  SkColor frame_color = SkColorSetA(
      background_color.value_or(GetColorProvider()->GetColor(
          active ? ui::kColorFrameActive : ui::kColorFrameInactive)),
      SK_AlphaOPAQUE);

  for (views::Button* button : {minimize_button(), maximize_button(),
                                restore_button(), close_button()}) {
    if (!button)
      continue;
    auto* frame_caption_button =
        static_cast<views::FrameCaptionButton*>(button);
    frame_caption_button->SetPaintAsActive(active);
    frame_caption_button->SetButtonColor(symbol_color);
    frame_caption_button->SetBackgroundColor(frame_color);
  }
}

void ElectronFrameViewLinux::
    UpdateCaptionButtonPlaceholderContainerBackground() {
  const std::optional<SkColor> obc = window_->overlay_button_color();
  SkColor bg_color;
  if (!obc.has_value()) {
    const auto* color_provider = GetColorProvider();
    if (!color_provider)
      return;
    bg_color = color_provider->GetColor(ShouldPaintAsActive()
                                            ? ui::kColorFrameActive
                                            : ui::kColorFrameInactive);
  } else {
    bg_color = *obc;
  }
  leading_button_container_->SetBackground(
      views::CreateSolidBackground(bg_color));
  trailing_button_container_->SetBackground(
      views::CreateSolidBackground(bg_color));
}

void ElectronFrameViewLinux::LayoutWindowControlsOverlay() {
  gfx::Rect overlay_rect = efv_layout()->GetWindowControlsOverlayRect();
  window_->SetWindowControlsOverlayRect(GetMirroredRect(overlay_rect));
  window_->NotifyLayoutWindowControlsOverlay();
}

BEGIN_METADATA(ElectronFrameViewLinux)
END_METADATA

}  // namespace electron
