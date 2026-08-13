// Copyright (c) 2026 Mitchell Cohen.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/freedesktop_nav_button_provider.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "base/check.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "shell/browser/ui/gtk/gtk_symbolic_icon_provider.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/image/image_skia_source.h"
#include "ui/views/window/frame_caption_button.h"

namespace electron {

namespace {

using ButtonState = ui::NavButtonProvider::ButtonState;
using DisplayType = ui::NavButtonProvider::FrameButtonDisplayType;

constexpr size_t Idx(ButtonState state) {
  return static_cast<size_t>(state);
}

static_assert(Idx(ButtonState::kDisabled) + 1 ==
                  std::tuple_size_v<decltype(ButtonStyle::states)>,
              "StateStyle arrays are indexed by ButtonState");

// Matches default height for Chromium vector icons in WCO.
constexpr int kDefaultBandHeight = 32;

constexpr int kNominalIconSize = 16;

// Freedesktop symbolic icon names.
constexpr std::array<const char*, 4> kIconNames = {
    "window-minimize-symbolic", "window-maximize-symbolic",
    "window-restore-symbolic", "window-close-symbolic"};
static_assert(static_cast<size_t>(DisplayType::kClose) + 1 ==
              kIconNames.size());

// ---------------------------------------------------------------------------
// Styles.
//
// Icon glyphs are monochrome and come directly from the Freedesktop
// symbolic icon theme installed on the system. (Different Linux distributions
// and desktop environments install glyphs for their default themes,
// e.g. Adwaita/Yaru/Breeze.)
//
// Styles allow further customization of the colors, sizing and spacing to
// match the look and feel of specific Linux desktop themes.
//
// ---------------------------------------------------------------------------

// Adwaita style (GNOME, also the fallback). Follows libadwaita's
// headerbar window buttons: a 24px circle of the foreground color
// at 10%/15%/30% opacity at rest/hover/pressed.
ButtonStyle AdwaitaButtonStyle() {
  ButtonStyle style;
  style.pill_diameter = 24;
  // libadwaita spaces window buttons by 3px of gap plus 5px of padding on
  // each button side (13px between circles), with ~10px from the window
  // edge to the outermost circle.
  style.button_margin = 5;
  style.inter_spacing = 3;
  style.end_spacing = 5;
  style.states = {{
      {PillColor::kBase, 0.10},  // normal
      {PillColor::kBase, 0.15},  // hovered
      {PillColor::kBase, 0.30},  // pressed
      {},                        // disabled
  }};
  style.close_states = style.states;
  return style;
}

// Breeze style (KDE Plasma). An 18px circle (Plasma's default grid unit),
// glyph only at rest, inverted colors on hover, and the palette's negative
// color for the close button (lightened on hover, darkened when pressed).
ButtonStyle BreezeButtonStyle() {
  constexpr SkColor kNegativeHover = SkColorSetRGB(0xFF, 0x98, 0xA2);
  constexpr SkColor kNegativePressed = SkColorSetRGB(0x6D, 0x22, 0x29);
  // KDE's small spacing: max(2px, grid unit / 4).
  constexpr int kSmallSpacing = 4;

  ButtonStyle style;
  style.pill_diameter = 18;
  // Plasma lays decoration buttons out with smallSpacing between them and
  // ~2x smallSpacing at the edge; split half of it into per-button margins
  // so the group also gets breathing room at its inner edge.
  style.button_margin = kSmallSpacing / 2;
  style.inter_spacing = kSmallSpacing - 2 * (kSmallSpacing / 2);
  style.end_spacing = 2 * kSmallSpacing - kSmallSpacing / 2;
  style.states = {{
      {},                                             // normal: glyph only
      {PillColor::kBase, 1.0, GlyphColor::kInverse},  // hovered
      {PillColor::kBase, 0.5, GlyphColor::kInverse},  // pressed
      {},                                             // disabled
  }};
  style.close_states = style.states;
  style.close_states[Idx(ButtonState::kHovered)] = {
      PillColor::kAccent, 1.0, GlyphColor::kInverse, kNegativeHover};
  style.close_states[Idx(ButtonState::kPressed)] = {
      PillColor::kAccent, 1.0, GlyphColor::kInverse, kNegativePressed};
  return style;
}

// Selects the style from the GTK theme name: Breeze themes get the Breeze
// treatment; everything else — including an unset or unknown theme — falls
// back to Adwaita, which is a common convention.
ButtonStyle DetectButtonStyle(const std::string& theme_name) {
  const bool breeze = base::StartsWith(theme_name, "Breeze",
                                       base::CompareCase::INSENSITIVE_ASCII);
  return breeze ? BreezeButtonStyle() : AdwaitaButtonStyle();
}

// ---------------------------------------------------------------------------
// Rendering.
// ---------------------------------------------------------------------------

class FreedesktopButtonImageSource : public gfx::ImageSkiaSource {
 public:
  FreedesktopButtonImageSource(SymbolicIconProvider* icon_provider,
                               DisplayType type,
                               const StateStyle& spec,
                               bool active,
                               int width,
                               int height,
                               int icon_size,
                               int pill_diameter,
                               std::optional<SkColor> background,
                               std::optional<SkColor> symbol)
      : icon_provider_(icon_provider),
        type_(type),
        spec_(spec),
        active_(active),
        width_(width),
        height_(height),
        icon_size_(icon_size),
        pill_diameter_(pill_diameter),
        background_(background),
        symbol_(symbol) {}

  ~FreedesktopButtonImageSource() override = default;

  gfx::ImageSkiaRep GetImageForScale(float scale) override {
    if (width_ <= 0 || height_ <= 0) {
      return gfx::ImageSkiaRep();
    }

    // Icon themes rasterize at integer scales. Render large and downscale
    // to the exact requested scale below so the result stays crisp on
    // fractional-scale displays.
    const int render_scale = std::ceil(scale);

    // Resolve the color roles. The base (glyph) color is the app's symbol
    // color when requested. Otherwise it is derived from the effective titlebar
    // background with the same contrast logic used by Chromium's vector icons
    const SkColor base =
        symbol_ ? *symbol_
        : background_
            ? views::FrameCaptionButton::GetAccessibleButtonColor(*background_)
            : SkColorSetRGB(0x2E, 0x34, 0x36);
    const SkColor inverse =
        background_ ? *background_ : color_utils::GetColorWithMaxContrast(base);
    const SkColor pill = spec_.pill == PillColor::kAccent ? spec_.accent : base;
    SkColor glyph = spec_.glyph == GlyphColor::kBase ? base
                    : spec_.glyph == GlyphColor::kInverse
                        ? inverse
                        : color_utils::GetColorWithMaxContrast(pill);
    if (!active_) {
      glyph = SkColorSetA(
          glyph,
          std::lround(
              SkColorGetA(glyph) *
              views::FrameCaptionButton::GetInactiveButtonColorAlphaRatio()));
    }

    const SkBitmap icon =
        icon_provider_->LoadIcon(kIconNames[static_cast<size_t>(type_)],
                                 icon_size_, render_scale, glyph);

    const int physical_width = render_scale * width_;
    const int physical_height = render_scale * height_;
    SkBitmap bitmap;
    bitmap.allocN32Pixels(physical_width, physical_height);
    bitmap.eraseColor(SK_ColorTRANSPARENT);
    SkCanvas canvas(bitmap);

    if (spec_.pill_alpha > 0) {
      SkPaint paint;
      paint.setAntiAlias(true);
      paint.setColor(
          SkColorSetA(pill, static_cast<U8CPU>(spec_.pill_alpha * 0xFF)));
      const int max_diameter = std::min(width_, height_);
      const int diameter = pill_diameter_ > 0
                               ? std::min(pill_diameter_, max_diameter)
                               : max_diameter;
      canvas.drawCircle(physical_width / 2.0f, physical_height / 2.0f,
                        render_scale * diameter / 2.0f, paint);
    }

    if (!icon.isNull()) {
      canvas.drawImage(icon.asImage(), (physical_width - icon.width()) / 2.0f,
                       (physical_height - icon.height()) / 2.0f);
    }

    if (scale != render_scale) {
      SkBitmap scaled;
      scaled.allocN32Pixels(std::lround(scale * width_),
                            std::lround(scale * height_));
      scaled.eraseColor(SK_ColorTRANSPARENT);
      bitmap.pixmap().scalePixels(
          scaled.pixmap(), SkSamplingOptions(SkCubicResampler::Mitchell()));
      bitmap = scaled;
    }

    return gfx::ImageSkiaRep(bitmap, scale);
  }

  bool HasRepresentationAtAllScales() const override { return true; }

 private:
  raw_ptr<SymbolicIconProvider> icon_provider_;
  DisplayType type_;
  StateStyle spec_;
  bool active_;
  int width_;
  int height_;
  int icon_size_;
  int pill_diameter_;
  std::optional<SkColor> background_;
  std::optional<SkColor> symbol_;
};

}  // namespace

// static
std::unique_ptr<FreedesktopNavButtonProvider>
FreedesktopNavButtonProvider::CreateIfAvailable() {
  // TODO(mitchchn): replace with LinuxUi call if/when available upstream.
  SymbolicIconProvider* icon_provider = GtkSymbolicIconProvider::GetInstance();
  if (!icon_provider) {
    return nullptr;
  }
  return base::WrapUnique(new FreedesktopNavButtonProvider(icon_provider));
}

FreedesktopNavButtonProvider::FreedesktopNavButtonProvider(
    SymbolicIconProvider* icon_provider)
    : icon_provider_(icon_provider) {
  style_ = DetectButtonStyle(icon_provider_->GetThemeName());
}

FreedesktopNavButtonProvider::~FreedesktopNavButtonProvider() = default;

void FreedesktopNavButtonProvider::RedrawImages(int top_area_height,
                                                bool maximized,
                                                bool active) {
  style_ = DetectButtonStyle(icon_provider_->GetThemeName());

  // Button bounds are full-height for hit targets when maximized/tiled
  // (Fitts' law) but they are only as wide as the icon pill.
  const int height = std::max(kNominalIconSize, top_area_height);
  const int width = style_.pill_diameter > 0
                        ? std::min(style_.pill_diameter, height)
                        : height;
  const int icon_size = std::min(kNominalIconSize, std::min(width, height) - 2);

  const gfx::Size button_size(width, height);
  for (auto type : {DisplayType::kMinimize,
                    maximized ? DisplayType::kRestore : DisplayType::kMaximize,
                    DisplayType::kClose}) {
    const auto& states =
        type == DisplayType::kClose ? style_.close_states : style_.states;
    for (auto state : {ButtonState::kNormal, ButtonState::kHovered,
                       ButtonState::kPressed, ButtonState::kDisabled}) {
      button_images_[type][state] =
          gfx::ImageSkia(std::make_unique<FreedesktopButtonImageSource>(
                             icon_provider_, type, states[Idx(state)], active,
                             width, height, icon_size, style_.pill_diameter,
                             titlebar_background_, symbol_color_),
                         button_size);
    }
  }
}

gfx::ImageSkia FreedesktopNavButtonProvider::GetImage(
    FrameButtonDisplayType type,
    ButtonState state) const {
  auto it = button_images_.find(type);
  CHECK(it != button_images_.end());
  auto it2 = it->second.find(state);
  CHECK(it2 != it->second.end());
  return it2->second;
}

gfx::Insets FreedesktopNavButtonProvider::GetNavButtonMargin(
    FrameButtonDisplayType type) const {
  return gfx::Insets::TLBR(0, style_.button_margin, 0, style_.button_margin);
}

gfx::Insets FreedesktopNavButtonProvider::GetTopAreaSpacing() const {
  // Horizontal spacing only: while native themes normally apply vertical
  // spacing, it would conflict with the app's WCO titlebar configuration.
  return gfx::Insets::TLBR(0, style_.end_spacing, 0, style_.end_spacing);
}

int FreedesktopNavButtonProvider::GetNavButtonHeight(bool maximized) const {
  return kDefaultBandHeight;
}

int FreedesktopNavButtonProvider::GetInterNavButtonSpacing() const {
  return style_.inter_spacing;
}

void FreedesktopNavButtonProvider::SetTitlebarColors(
    std::optional<SkColor> background,
    std::optional<SkColor> symbol) {
  titlebar_background_ = background;
  symbol_color_ = symbol;
}

}  // namespace electron
