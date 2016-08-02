// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_screen.h"

#include <algorithm>
#include <string>

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "base/bind.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

// Find an item in container according to its ID.
template<class T>
typename T::iterator FindById(T* container, int id) {
  auto predicate = [id] (const typename T::value_type& item) -> bool {
    return item.id() == id;
  };
  return std::find_if(container->begin(), container->end(), predicate);
}

// Convert the changed_metrics bitmask to string array.
std::vector<std::string> MetricsToArray(uint32_t metrics) {
  std::vector<std::string> array;
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_BOUNDS)
    array.push_back("bounds");
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_WORK_AREA)
    array.push_back("workArea");
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_DEVICE_SCALE_FACTOR)
    array.push_back("scaleFactor");
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_ROTATION)
    array.push_back("rotation");
  return array;
}

}  // namespace

Screen::Screen(v8::Isolate* isolate, display::Screen* screen)
    : screen_(screen) {
  screen_->AddObserver(this);
  Init(isolate);
}

Screen::~Screen() {
  screen_->RemoveObserver(this);
}

gfx::Point Screen::GetCursorScreenPoint() {
  return screen_->GetCursorScreenPoint();
}

display::Display Screen::GetPrimaryDisplay() {
  return screen_->GetPrimaryDisplay();
}

std::vector<display::Display> Screen::GetAllDisplays() {
  return screen_->GetAllDisplays();
}

display::Display Screen::GetDisplayNearestPoint(const gfx::Point& point) {
  return screen_->GetDisplayNearestPoint(point);
}

display::Display Screen::GetDisplayMatching(const gfx::Rect& match_rect) {
  return screen_->GetDisplayMatching(match_rect);
}

void Screen::OnDisplayAdded(const display::Display& new_display) {
  Emit("display-added", new_display);
}

void Screen::OnDisplayRemoved(const display::Display& old_display) {
  Emit("display-removed", old_display);
}

void Screen::OnDisplayMetricsChanged(const display::Display& display,
                                     uint32_t changed_metrics) {
  Emit("display-metrics-changed", display, MetricsToArray(changed_metrics));
}

// static
v8::Local<v8::Value> Screen::Create(v8::Isolate* isolate) {
  if (!Browser::Get()->is_ready()) {
    isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
        isolate,
        "Cannot initialize \"screen\" module before app is ready")));
    return v8::Null(isolate);
  }

  display::Screen* screen = display::Screen::GetScreen();
  if (!screen) {
    isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
        isolate, "Failed to get screen information")));
    return v8::Null(isolate);
  }

  return mate::CreateHandle(isolate, new Screen(isolate, screen)).ToV8();
}

// static
void Screen::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("getCursorScreenPoint", &Screen::GetCursorScreenPoint)
      .SetMethod("getPrimaryDisplay", &Screen::GetPrimaryDisplay)
      .SetMethod("getAllDisplays", &Screen::GetAllDisplays)
      .SetMethod("getDisplayNearestPoint", &Screen::GetDisplayNearestPoint)
      .SetMethod("getDisplayMatching", &Screen::GetDisplayMatching);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.Set("screen", atom::api::Screen::Create(context->GetIsolate()));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_screen, Initialize)
