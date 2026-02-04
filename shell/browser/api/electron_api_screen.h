// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SCREEN_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SCREEN_H_

#include <vector>

#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "ui/display/display_observer.h"
#include "ui/display/screen.h"

namespace gfx {
class Point;
class PointF;
class Rect;
}  // namespace gfx

namespace gin_helper {
class ErrorThrower;
}  // namespace gin_helper

namespace electron::api {

class Screen final : public gin::Wrappable<Screen>,
                     public gin_helper::EventEmitterMixin<Screen>,
                     private display::DisplayObserver {
 public:
  static Screen* Create(gin_helper::ErrorThrower error_thrower);

  static const gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  const char* GetClassName() const { return "Screen"; }

  // disable copy
  Screen(const Screen&) = delete;
  Screen& operator=(const Screen&) = delete;

  // Make public for cppgc::MakeGarbageCollected.
  Screen();
  ~Screen() override;

  [[nodiscard]] gfx::Point GetCursorScreenPoint(v8::Isolate* isolate);
  [[nodiscard]] display::Display GetPrimaryDisplay() const;
  [[nodiscard]] std::vector<display::Display> GetAllDisplays() const;
  [[nodiscard]] display::Display GetDisplayNearestPoint(
      const gfx::Point& point) const;
  [[nodiscard]] display::Display GetDisplayMatching(
      const gfx::Rect& match_rect) const;

  gfx::PointF ScreenToDIPPoint(const gfx::PointF& point_px);
  gfx::Point DIPToScreenPoint(const gfx::Point& point_dip);

  // display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplaysRemoved(const display::Displays& removed_displays) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SCREEN_H_
