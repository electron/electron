// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_screen.h"

#include <algorithm>
#include <string>

#include "atom/browser/api/atom_api_browser_window.h"
#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "base/bind.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"

#if defined(OS_WIN)
#include "ui/display/win/screen_win.h"
#endif

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

// Find an item in container according to its ID.
template <class T>
typename T::iterator FindById(T* container, int id) {
  auto predicate = [id](const typename T::value_type& item) -> bool {
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

void DelayEmit(Screen* screen,
               const base::StringPiece& name,
               const display::Display& display) {
  screen->Emit(name, display);
}

void DelayEmitWithMetrics(Screen* screen,
                          const base::StringPiece& name,
                          const display::Display& display,
                          const std::vector<std::string>& metrics) {
  screen->Emit(name, display, metrics);
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

#if defined(OS_WIN)

static gfx::Rect ScreenToDIPRect(atom::NativeWindow* window,
                                 const gfx::Rect& rect) {
  HWND hwnd = window ? window->GetAcceleratedWidget() : nullptr;
  return display::win::ScreenWin::ScreenToDIPRect(hwnd, rect);
}

static gfx::Rect DIPToScreenRect(atom::NativeWindow* window,
                                 const gfx::Rect& rect) {
  HWND hwnd = window ? window->GetAcceleratedWidget() : nullptr;
  return display::win::ScreenWin::DIPToScreenRect(hwnd, rect);
}

#endif

void Screen::OnDisplayAdded(const display::Display& new_display) {
  base::ThreadTaskRunnerHandle::Get()->PostNonNestableTask(
      FROM_HERE, base::Bind(&DelayEmit, base::Unretained(this), "display-added",
                            new_display));
}

void Screen::OnDisplayRemoved(const display::Display& old_display) {
  base::ThreadTaskRunnerHandle::Get()->PostNonNestableTask(
      FROM_HERE, base::Bind(&DelayEmit, base::Unretained(this),
                            "display-removed", old_display));
}

void Screen::OnDisplayMetricsChanged(const display::Display& display,
                                     uint32_t changed_metrics) {
  base::ThreadTaskRunnerHandle::Get()->PostNonNestableTask(
      FROM_HERE, base::Bind(&DelayEmitWithMetrics, base::Unretained(this),
                            "display-metrics-changed", display,
                            MetricsToArray(changed_metrics)));
}

// static
v8::Local<v8::Value> Screen::Create(v8::Isolate* isolate) {
  if (!Browser::Get()->is_ready()) {
    isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
        isolate, "Cannot require \"screen\" module before app is ready")));
    return v8::Null(isolate);
  }

  display::Screen* screen = display::Screen::GetScreen();
  if (!screen) {
    isolate->ThrowException(v8::Exception::Error(
        mate::StringToV8(isolate, "Failed to get screen information")));
    return v8::Null(isolate);
  }

  return mate::CreateHandle(isolate, new Screen(isolate, screen)).ToV8();
}

// static
void Screen::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Screen"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("getCursorScreenPoint", &Screen::GetCursorScreenPoint)
      .SetMethod("getPrimaryDisplay", &Screen::GetPrimaryDisplay)
      .SetMethod("getAllDisplays", &Screen::GetAllDisplays)
      .SetMethod("getDisplayNearestPoint", &Screen::GetDisplayNearestPoint)
#if defined(OS_WIN)
      .SetMethod("screenToDipPoint", &display::win::ScreenWin::ScreenToDIPPoint)
      .SetMethod("dipToScreenPoint", &display::win::ScreenWin::DIPToScreenPoint)
      .SetMethod("screenToDipRect", &ScreenToDIPRect)
      .SetMethod("dipToScreenRect", &DIPToScreenRect)
#endif
      .SetMethod("getDisplayMatching", &Screen::GetDisplayMatching);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::Screen;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("screen", Screen::Create(isolate));
  dict.Set(
      "Screen",
      Screen::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_common_screen, Initialize)
