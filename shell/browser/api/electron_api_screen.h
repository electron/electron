// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SCREEN_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SCREEN_H_

#include <vector>

#include "gin/wrappable.h"
#include "shell/browser/browser_observer.h"
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
                     private BrowserObserver,
                     private display::DisplayObserver {
 public:
  static Screen* Create(v8::Isolate* isolate);

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

  // These throw until the app is ready. They are static so that they work
  // detached from the screen object.
  [[nodiscard]] static gfx::Point GetCursorScreenPoint(
      gin_helper::ErrorThrower thrower);
  [[nodiscard]] static display::Display GetPrimaryDisplay(
      gin_helper::ErrorThrower thrower);
  [[nodiscard]] static std::vector<display::Display> GetAllDisplays(
      gin_helper::ErrorThrower thrower);
  [[nodiscard]] static display::Display GetDisplayNearestPoint(
      gin_helper::ErrorThrower thrower,
      const gfx::Point& point);
  [[nodiscard]] static display::Display GetDisplayMatching(
      gin_helper::ErrorThrower thrower,
      const gfx::Rect& match_rect);

  static gfx::PointF ScreenToDIPPoint(gin_helper::ErrorThrower thrower,
                                      const gfx::PointF& point_px);
  static gfx::Point DIPToScreenPoint(gin_helper::ErrorThrower thrower,
                                     const gfx::Point& point_dip);

 private:
  // Throws and returns false before the app is ready.
  static bool CheckReady(gin_helper::ErrorThrower thrower);
  void ObserveDisplays();

  // BrowserObserver:
  void OnWillFinishLaunching() override;
  void OnFinishLaunching(base::DictValue launch_info) override;

  // display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplaysRemoved(const display::Displays& removed_displays) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  bool display_observation_started_ = false;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SCREEN_H_
