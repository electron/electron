// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_API_ELECTRON_API_SCREEN_H_
#define ELECTRON_BROWSER_API_ELECTRON_API_SCREEN_H_

#include <vector>

#include "electron/browser/api/event_emitter.h"
#include "native_mate/handle.h"
#include "ui/gfx/display_observer.h"

namespace gfx {
class Point;
class Rect;
class Screen;
}

namespace electron {

namespace api {

class Screen : public mate::EventEmitter,
               public gfx::DisplayObserver {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate);

 protected:
  explicit Screen(gfx::Screen* screen);
  virtual ~Screen();

  gfx::Point GetCursorScreenPoint();
  gfx::Display GetPrimaryDisplay();
  std::vector<gfx::Display> GetAllDisplays();
  gfx::Display GetDisplayNearestPoint(const gfx::Point& point);
  gfx::Display GetDisplayMatching(const gfx::Rect& match_rect);

  // gfx::DisplayObserver:
  void OnDisplayAdded(const gfx::Display& new_display) override;
  void OnDisplayRemoved(const gfx::Display& old_display) override;
  void OnDisplayMetricsChanged(const gfx::Display& display,
                               uint32_t changed_metrics) override;

  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  gfx::Screen* screen_;
  std::vector<gfx::Display> displays_;

  DISALLOW_COPY_AND_ASSIGN(Screen);
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_BROWSER_API_ELECTRON_API_SCREEN_H_
