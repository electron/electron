// Copyright (c) 2025 Mitchell Cohen.
// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_FRAME_LAYOUT_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_FRAME_LAYOUT_H_

#include <memory>

#include "base/i18n/rtl.h"
#include "shell/browser/linux/x11_util.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/electron_desktop_window_tree_host_linux.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "ui/base/ozone_buildflags.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/linux/linux_ui.h"
#include "ui/linux/window_frame_provider.h"

namespace electron {

class NativeWindowViews;

// Shared helper for CSD layout and frame painting on Linux (shadows, resize
// regions, titlebars, etc.). Also helps views determine insets and perform
// bounds conversions between widget and logical coordinates.
class LinuxFrameLayout {
 public:
  virtual ~LinuxFrameLayout() = default;

  static std::unique_ptr<LinuxFrameLayout> Create(NativeWindowViews* window,
                                                  bool wants_shadow);

  // Insets from the transparent widget border to the opaque part of the window
  virtual gfx::Insets RestoredFrameBorderInsets() const = 0;
  // Insets for parts of the surface that should be counted for user input
  virtual gfx::Insets GetInputInsets() const = 0;

  virtual bool SupportsClientFrameShadow() const = 0;

  virtual bool tiled() const = 0;
  virtual void set_tiled(bool tiled) = 0;

  virtual void PaintWindowFrame(gfx::Canvas* canvas,
                                gfx::Rect local_bounds,
                                gfx::Rect titlebar_bounds,
                                bool active) = 0;

  // The logical bounds of the window
  virtual gfx::Rect GetWindowContentBounds() const = 0;
  // The logical bounds as a rounded rect with corner radii applied
  virtual SkRRect GetRoundedWindowContentBounds() const = 0;

  virtual int GetTranslucentTopAreaHeight() const = 0;

  virtual ui::WindowFrameProvider* GetFrameProvider() const = 0;
};

// Client-side decoration (CSD) Linux frame layout implementation.
class LinuxCSDFrameLayout : public LinuxFrameLayout {
 public:
  explicit LinuxCSDFrameLayout(NativeWindowViews* window);
  ~LinuxCSDFrameLayout() override = default;

  gfx::Insets RestoredFrameBorderInsets() const override;
  gfx::Insets GetInputInsets() const override;
  bool SupportsClientFrameShadow() const override;
  bool tiled() const override;
  void set_tiled(bool tiled) override;
  void PaintWindowFrame(gfx::Canvas* canvas,
                        gfx::Rect local_bounds,
                        gfx::Rect titlebar_bounds,
                        bool active) override;
  gfx::Rect GetWindowContentBounds() const override;
  SkRRect GetRoundedWindowContentBounds() const override;
  int GetTranslucentTopAreaHeight() const override;
  ui::WindowFrameProvider* GetFrameProvider() const override;

 private:
  raw_ptr<NativeWindowViews> window_;
  bool tiled_ = false;
  bool host_supports_client_frame_shadow_ = false;
};

// No-decoration Linux frame layout implementation.
//
// Intended for cases where we do not allocate a transparent inset area around
// the window (e.g. X11 / server-side decorations, or when insets are disabled).
// All inset math returns 0 and frame painting is skipped.
class LinuxUndecoratedFrameLayout : public LinuxFrameLayout {
 public:
  explicit LinuxUndecoratedFrameLayout(NativeWindowViews* window);
  ~LinuxUndecoratedFrameLayout() override = default;

  gfx::Insets RestoredFrameBorderInsets() const override;
  gfx::Insets GetInputInsets() const override;
  bool SupportsClientFrameShadow() const override;
  bool tiled() const override;
  void set_tiled(bool tiled) override;
  void PaintWindowFrame(gfx::Canvas* canvas,
                        gfx::Rect local_bounds,
                        gfx::Rect titlebar_bounds,
                        bool active) override;
  gfx::Rect GetWindowContentBounds() const override;
  SkRRect GetRoundedWindowContentBounds() const override;
  int GetTranslucentTopAreaHeight() const override;
  ui::WindowFrameProvider* GetFrameProvider() const override;

 private:
  raw_ptr<NativeWindowViews> window_;
  bool tiled_ = false;
};
}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_FRAME_LAYOUT_H_
