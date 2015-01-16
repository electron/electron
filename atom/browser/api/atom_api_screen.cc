// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_screen.h"

#include <algorithm>

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "base/bind.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "ui/gfx/screen.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

template<class T>
typename T::iterator FindById(T* container, int id) {
  auto predicate = [id] (const typename T::value_type& item) -> bool {
    return item.id() == id;
  };
  return std::find_if(container->begin(), container->end(), predicate);
}

}  // namespace

Screen::Screen(gfx::Screen* screen) : screen_(screen) {
  screen_->AddObserver(this);
}

Screen::~Screen() {
  screen_->RemoveObserver(this);
}

gfx::Point Screen::GetCursorScreenPoint() {
  return screen_->GetCursorScreenPoint();
}

gfx::Display Screen::GetPrimaryDisplay() {
  return screen_->GetPrimaryDisplay();
}

std::vector<gfx::Display> Screen::GetAllDisplays() {
  if (displays_.size() == 0)
    displays_ = screen_->GetAllDisplays();
  return displays_;
}

gfx::Display Screen::GetDisplayNearestPoint(const gfx::Point& point) {
  return screen_->GetDisplayNearestPoint(point);
}

gfx::Display Screen::GetDisplayMatching(const gfx::Rect& match_rect) {
  return screen_->GetDisplayMatching(match_rect);
}

void Screen::OnDisplayAdded(const gfx::Display& new_display) {
  displays_.push_back(new_display);
  Emit("display-added");
}

void Screen::OnDisplayRemoved(const gfx::Display& old_display) {
  displays_.erase(FindById(&displays_, old_display.id()));
  Emit("display-removed");
}

void Screen::OnDisplayMetricsChanged(const gfx::Display& display,
                                     uint32_t changed_metrics) {
  Emit("display-metrics-changed");
}

mate::ObjectTemplateBuilder Screen::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("getCursorScreenPoint", &Screen::GetCursorScreenPoint)
      .SetMethod("getPrimaryDisplay", &Screen::GetPrimaryDisplay)
      .SetMethod("getAllDisplays", &Screen::GetAllDisplays)
      .SetMethod("getDisplayNearestPoint", &Screen::GetDisplayNearestPoint)
      .SetMethod("getDisplayMatching", &Screen::GetDisplayMatching);
}

// static
mate::Handle<Screen> Screen::Create(v8::Isolate* isolate) {
  gfx::Screen* screen = gfx::Screen::GetNativeScreen();
  if (!screen) {
    isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
        isolate, "Failed to get screen information")));
    return mate::Handle<Screen>();
  }

  return CreateHandle(isolate, new Screen(screen));
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.Set("screen", atom::api::Screen::Create(context->GetIsolate()));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_screen, Initialize)
