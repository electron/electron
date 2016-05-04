// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SCREEN_H_
#define ATOM_BROWSER_API_ATOM_API_SCREEN_H_

#include <vector>

#include "atom/browser/api/event_emitter.h"
#include "native_mate/handle.h"
#include "ui/gfx/display_observer.h"

namespace gfx {
class Point;
class Rect;
class Screen;
}

namespace atom {

namespace api {

class Screen : public mate::EventEmitter<Screen>,
               public gfx::DisplayObserver {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  Screen(v8::Isolate* isolate, gfx::Screen* screen);
  ~Screen() override;

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

 private:
  gfx::Screen* screen_;

  DISALLOW_COPY_AND_ASSIGN(Screen);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SCREEN_H_
