// Copyright (c) 2026 Athul Iddya.
// Copyright (c) 2024 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LINUX_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LINUX_H_

#include <memory>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/native_frame_view_linux.h"

class CaptionButtonPlaceholderContainer;

namespace electron {

class FreedesktopNavButtonProvider;
class NativeWindowViews;
class ElectronFrameViewLayoutLinux;

// View used for frame: false with optional WCO.
//
// TODO(mitchchn): Rebase on FrameViewLinux after it is refactored upstream
// to accept a nav button provider, since NativeFrameViewLinux
// is supposed to be used with a frame provider, which we do not need here.
class ElectronFrameViewLinux : public views::NativeFrameViewLinux {
  METADATA_HEADER(ElectronFrameViewLinux, views::NativeFrameViewLinux)

 public:
  ElectronFrameViewLinux(
      NativeWindowViews* window,
      views::Widget* widget,
      std::unique_ptr<ui::NavButtonProvider> nav_button_provider,
      ElectronFrameViewLayoutLinux* layout,
      FreedesktopNavButtonProvider* freedesktop_provider = nullptr);
  ~ElectronFrameViewLinux() override;

  NativeWindowViews* window() const { return window_; }

  bool WantsFrame() const;
  void SetWantsFrame(bool wants_frame);

  // views::FrameViewLinux:
  bool HasWindowTitle() const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void Layout(PassKey) override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;

 protected:
  // views::FrameViewLinux:
  void CreateCaptionButtons() override;
  void PaintRestoredFrameBorder(gfx::Canvas* canvas) override;
  void PaintMaximizedFrameBorder(gfx::Canvas* canvas) override;
  void UpdateButtonColors() override;

 private:
  ElectronFrameViewLayoutLinux* efv_layout() const;
  void UpdateCaptionButtonPlaceholderContainerBackground();
  void LayoutWindowControlsOverlay();

  raw_ptr<NativeWindowViews> window_;

  raw_ptr<FreedesktopNavButtonProvider> freedesktop_provider_ = nullptr;

  std::optional<SkColor> last_titlebar_background_;
  std::optional<SkColor> last_symbol_color_;

  // Background views behind the leading and trailing caption buttons.
  raw_ptr<CaptionButtonPlaceholderContainer> leading_button_container_ =
      nullptr;
  raw_ptr<CaptionButtonPlaceholderContainer> trailing_button_container_ =
      nullptr;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_ELECTRON_FRAME_VIEW_LINUX_H_
