// Copyright (c) 2026 Mitchell Cohen.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_FREEDESKTOP_NAV_BUTTON_PROVIDER_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_FREEDESKTOP_NAV_BUTTON_PROVIDER_H_

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/linux/nav_button_provider.h"

class SkBitmap;

namespace electron {

// Allows toolkit-specific impls to load Freedesktop icons and
// provide system theme information.
class SymbolicIconProvider {
 public:
  virtual SkBitmap LoadIcon(const std::string& icon_name,
                            int icon_size,
                            int scale,
                            SkColor color) = 0;
  virtual std::string GetThemeName() = 0;

 protected:
  ~SymbolicIconProvider() = default;
};

enum class PillColor { kBase, kAccent };
enum class GlyphColor { kBase, kInverse, kContrastWithPill };

struct StateStyle {
  PillColor pill = PillColor::kBase;
  double pill_alpha = 0.0;
  GlyphColor glyph = GlyphColor::kBase;
  SkColor accent = SK_ColorTRANSPARENT;
};

// Button styles customize the appearance and metrics of caption buttons
// to support different themes.
struct ButtonStyle {
  // Diameter of the circular highlight; 0 means the full button size.
  int pill_diameter = 0;
  // Horizontal spacing is composed the way desktop headerbars compose it:
  int button_margin = 0;
  int inter_spacing = 0;
  int end_spacing = 0;
  std::array<StateStyle, 4> states;
  std::array<StateStyle, 4> close_states;
};

// A NavButtonProvider that draws caption buttons using freedesktop -symbolic
// icons.
class FreedesktopNavButtonProvider : public ui::NavButtonProvider {
 public:
  static std::unique_ptr<FreedesktopNavButtonProvider> CreateIfAvailable();

  FreedesktopNavButtonProvider(const FreedesktopNavButtonProvider&) = delete;
  FreedesktopNavButtonProvider& operator=(const FreedesktopNavButtonProvider&) =
      delete;
  ~FreedesktopNavButtonProvider() override;

  // ui::NavButtonProvider:
  void RedrawImages(int top_area_height, bool maximized, bool active) override;
  gfx::ImageSkia GetImage(FrameButtonDisplayType type,
                          ButtonState state) const override;
  gfx::Insets GetNavButtonMargin(FrameButtonDisplayType type) const override;
  gfx::Insets GetTopAreaSpacing() const override;
  int GetNavButtonHeight(bool maximized) const override;
  int GetInterNavButtonSpacing() const override;

  void SetTitlebarColors(std::optional<SkColor> background,
                         std::optional<SkColor> symbol);

 private:
  explicit FreedesktopNavButtonProvider(SymbolicIconProvider* icon_provider);

  // Should be process lifetime (ideally provided by the LinuxUi singleton)
  // since the ImageSkia passed to the caption buttons can call the icon
  // provider lazily and can outlive the nav button provider.
  raw_ptr<SymbolicIconProvider> icon_provider_;

  std::map<FrameButtonDisplayType, std::map<ButtonState, gfx::ImageSkia>>
      button_images_;

  ButtonStyle style_;
  std::optional<SkColor> titlebar_background_;
  std::optional<SkColor> symbol_color_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_FREEDESKTOP_NAV_BUTTON_PROVIDER_H_
