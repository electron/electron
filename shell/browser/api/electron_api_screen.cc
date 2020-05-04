// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_screen.h"

#include <algorithm>
#include <string>

#include "base/bind.h"
#include "gin/dictionary.h"
#include "gin/handle.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/native_window_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"

#if defined(OS_WIN)
#include "ui/display/win/screen_win.h"
#endif

namespace electron {

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
    array.emplace_back("bounds");
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_WORK_AREA)
    array.emplace_back("workArea");
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_DEVICE_SCALE_FACTOR)
    array.emplace_back("scaleFactor");
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_ROTATION)
    array.emplace_back("rotation");
  return array;
}

void DelayEmit(Screen* screen,
               base::StringPiece name,
               const display::Display& display) {
  screen->Emit(name, display);
}

void DelayEmitWithMetrics(Screen* screen,
                          base::StringPiece name,
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

static gfx::Rect ScreenToDIPRect(electron::NativeWindow* window,
                                 const gfx::Rect& rect) {
  HWND hwnd = window ? window->GetAcceleratedWidget() : nullptr;
  return display::win::ScreenWin::ScreenToDIPRect(hwnd, rect);
}

static gfx::Rect DIPToScreenRect(electron::NativeWindow* window,
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
v8::Local<v8::Value> Screen::Create(gin_helper::ErrorThrower error_thrower) {
  if (!Browser::Get()->is_ready()) {
    error_thrower.ThrowError(
        "The 'screen' module can't be used before the app 'ready' event");
    return v8::Null(error_thrower.isolate());
  }

  display::Screen* screen = display::Screen::GetScreen();
  if (!screen) {
    error_thrower.ThrowError("Failed to get screen information");
    return v8::Null(error_thrower.isolate());
  }

  return gin::CreateHandle(error_thrower.isolate(),
                           new Screen(error_thrower.isolate(), screen))
      .ToV8();
}

// static
void Screen::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "Screen"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
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

}  // namespace electron

namespace {

using electron::api::Screen;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("createScreen", base::BindRepeating(&Screen::Create));
  dict.Set(
      "Screen",
      Screen::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_screen, Initialize)
