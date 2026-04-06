// Copyright (c) 2026 Athul Iddya.
// Copyright (c) 2024 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/electron_frame_view_linux.h"

#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/caption_button_placeholder_container.h"
#include "shell/browser/ui/views/electron_frame_view_layout_linux.h"
#include "ui/base/hit_test.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/views/background.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/frame_caption_button.h"

namespace electron {

ElectronFrameViewLinux::ElectronFrameViewLinux(NativeWindowViews* window,
                                               views::Widget* widget)
    : FrameViewLinux(widget,
                     new ElectronFrameViewLayoutLinux(
                         window,
                         window->IsWindowControlsOverlayEnabled())),
      window_(window) {
  InitViews();
}

ElectronFrameViewLinux::~ElectronFrameViewLinux() = default;

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

  FrameViewLinux::CreateCaptionButtons();

  // Promote to layers so buttons render above the overlapping client view.
  for (auto* btn : {minimize_button(), maximize_button(), restore_button(),
                    close_button()}) {
    if (btn) {
      btn->SetPaintToLayer();
      btn->layer()->SetFillsBoundsOpaquely(false);
    }
  }
}

int ElectronFrameViewLinux::NonClientHitTest(const gfx::Point& point) {
  int result = FrameViewLinux::NonClientHitTest(point);
  if (result != HTCLIENT)
    return result;

  // The base class returns HTCLIENT for the WCO titlebar area because the
  // client view overlaps it. Check buttons and the draggable overlay region.
  if (window_->IsWindowControlsOverlayEnabled()) {
    using Type = ui::NavButtonProvider::FrameButtonDisplayType;
    struct {
      Type type;
      int code;
    } buttons[] = {
        {Type::kClose, HTCLOSE},
        {Type::kRestore, HTMAXBUTTON},
        {Type::kMaximize, HTMAXBUTTON},
        {Type::kMinimize, HTMINBUTTON},
    };
    for (const auto& [type, code] : buttons) {
      auto* btn = GetButtonFromType(type);
      if (btn && btn->GetVisible() && btn->GetMirroredBounds().Contains(point))
        return code;
    }
    if (efv_layout()->GetLeadingButtonRect().Contains(point) ||
        efv_layout()->GetTrailingButtonRect().Contains(point))
      return HTCAPTION;
  }

  if (window_) {
    int contents_hit_test = window_->NonClientHitTest(point);
    if (contents_hit_test != HTNOWHERE)
      return contents_hit_test;
  }

  return HTCLIENT;
}

void ElectronFrameViewLinux::Layout(PassKey) {
  LayoutSuperclass<FrameViewLinux>(this);

  if (!window_->IsWindowControlsOverlayEnabled())
    return;

  leading_button_container_->SetBoundsRect(
      efv_layout()->GetLeadingButtonRect());
  trailing_button_container_->SetBoundsRect(
      efv_layout()->GetTrailingButtonRect());

  UpdateCaptionButtonPlaceholderContainerBackground();
  LayoutWindowControlsOverlay();
}

void ElectronFrameViewLinux::PaintRestoredFrameBorder(gfx::Canvas* canvas) {
  if (!WantsFrame())
    return;
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
  if (!window_->IsWindowControlsOverlayEnabled())
    return;

  // Apply custom WCO overlay colors if set.
  const bool active = ShouldPaintAsActive();
  const SkColor symbol_color = window_->overlay_symbol_color();
  const SkColor background_color = window_->overlay_button_color();
  SkColor frame_color =
      background_color == SkColor()
          ? GetColorProvider()->GetColor(active ? ui::kColorFrameActive
                                                : ui::kColorFrameInactive)
          : background_color;

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
  const SkColor obc = window_->overlay_button_color();
  SkColor bg_color;
  if (obc == SkColor()) {
    const auto* color_provider = GetColorProvider();
    if (!color_provider)
      return;
    bg_color = color_provider->GetColor(ShouldPaintAsActive()
                                            ? ui::kColorFrameActive
                                            : ui::kColorFrameInactive);
  } else {
    bg_color = obc;
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
