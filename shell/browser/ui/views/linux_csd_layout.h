// Copyright (c) 2025 Mitchell Cohen.
// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_CSD_LAYOUT_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_CSD_LAYOUT_H_

#include "base/i18n/rtl.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/electron_desktop_window_tree_host_linux.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/linux/linux_ui.h"
#include "ui/linux/window_frame_provider.h"

namespace electron {

class NativeWindowViews;

// Shared helper for CSD layout and frame painting on Linux (shadows, resize
// regions, titlebars, etc.). Also helps views determine insets and perform
// bounds conversions between widget and logical coordinates.
class LinuxCSDLayout {
 public:
  LinuxCSDLayout(NativeWindowViews* window);
  ~LinuxCSDLayout() = default;

  // Insets from the transparent widget border to the opaque part of the window
  gfx::Insets RestoredFrameBorderInsets() const;
  // Insets for parts of the surface that should be counted for user input
  gfx::Insets GetInputInsets() const;

  bool SupportsClientFrameShadow() const;

  bool tiled() const { return tiled_; }
  void set_tiled(bool tiled) { tiled_ = tiled; }

  void PaintWindowFrame(gfx::Canvas* canvas,
                        gfx::Rect local_bounds,
                        gfx::Rect titlebar_bounds,
                        bool active);

  // The logical bounds of the window
  gfx::Rect GetWindowContentBounds() const;
  // The logical bounds as a rounded rect with corner radii applied
  SkRRect GetRoundedWindowContentBounds() const;

  int GetTranslucentTopAreaHeight() const;

  ui::WindowFrameProvider* GetFrameProvider() const;

 private:
  raw_ptr<NativeWindowViews> window_;
  bool tiled_ = false;
  bool host_supports_client_frame_shadow_ = false;
};
}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_CSD_LAYOUT_H_
