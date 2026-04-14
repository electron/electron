// Copyright (c) 2025 Mitchell Cohen.
// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_FRAME_LAYOUT_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_FRAME_LAYOUT_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "ui/gfx/shadow_value.h"
#include "ui/linux/window_frame_provider.h"

namespace gfx {
class Insets;
class Rect;
}  // namespace gfx

namespace electron {

class NativeWindowViews;

// Shared helper for CSD layout on Linux (shadows, resize regions, titlebars,
// etc.). Also helps views determine insets and perform bounds conversions
// between widget and logical coordinates.
//
// The base class is concrete and suitable as-is for the undecorated case (X11,
// translucent windows, or windows without shadows). CSD subclasses override
// the methods that differ.
class LinuxFrameLayout {
 public:
  enum class CSDStyle {
    kNativeFrame,
    kCustom,
  };

  explicit LinuxFrameLayout(NativeWindowViews* window);
  virtual ~LinuxFrameLayout();

  static std::unique_ptr<LinuxFrameLayout> Create(NativeWindowViews* window,
                                                  bool wants_shadow,
                                                  CSDStyle csd_style);

  // Insets from the transparent widget border to the opaque part of the window.
  // Returns empty insets when maximized or fullscreen unless |restored| is
  // true. Matches Chromium's OpaqueBrowserFrameViewLayout::FrameBorderInsets.
  gfx::Insets FrameBorderInsets(bool restored) const;
  virtual gfx::Insets RestoredFrameBorderInsets() const;
  // Insets for parts of the surface that should be counted for user input.
  virtual gfx::Insets GetInputInsets() const;
  // Insets to use for non-client resize hit-testing.
  gfx::Insets GetResizeBorderInsets() const;

  bool IsShowingShadow() const;
  bool SupportsClientFrameShadow() const;

  bool tiled() const;
  void set_tiled(bool tiled);

  // The logical bounds of the window interior.
  gfx::Rect GetWindowBounds() const;
  // The logical window bounds as a rounded rect with corner radii applied.
  SkRRect GetRoundedWindowBounds() const;
  // The corner radius of the top corners of the window, in DIPs.
  virtual float GetTopCornerRadiusDip() const;

  int GetTranslucentTopAreaHeight() const;

 protected:
  gfx::Insets NormalizeBorderInsets(const gfx::Insets& frame_insets,
                                    const gfx::Insets& input_insets) const;

  raw_ptr<NativeWindowViews> window_;
  bool tiled_ = false;
  bool host_supports_client_frame_shadow_ = false;
};

// CSD strategy that uses the GTK window frame provider for metrics.
class LinuxCSDNativeFrameLayout : public LinuxFrameLayout {
 public:
  explicit LinuxCSDNativeFrameLayout(NativeWindowViews* window);
  ~LinuxCSDNativeFrameLayout() override;

  gfx::Insets RestoredFrameBorderInsets() const override;
  gfx::Insets GetInputInsets() const override;
  float GetTopCornerRadiusDip() const override;
  ui::WindowFrameProvider* GetFrameProvider() const;
};

// CSD strategy that uses custom metrics, similar to those used in Chromium.
class LinuxCSDCustomFrameLayout : public LinuxFrameLayout {
 public:
  explicit LinuxCSDCustomFrameLayout(NativeWindowViews* window);
  ~LinuxCSDCustomFrameLayout() override;

  gfx::Insets RestoredFrameBorderInsets() const override;
  gfx::Insets GetInputInsets() const override;
};

gfx::ShadowValues GetFrameShadowValuesLinux(bool active);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_FRAME_LAYOUT_H_
