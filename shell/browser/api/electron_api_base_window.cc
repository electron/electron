// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_base_window.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/task/single_thread_task_runner.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "gin/dictionary.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/browser/api/electron_api_view.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/color_util.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_converters/native_window_converter.h"
#include "shell/common/gin_converters/optional_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_template.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/persistent_dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"

#if BUILDFLAG(IS_MAC)
#include "shell/browser/api/electron_api_app_shim_host.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "shell/browser/native_window_views.h"
#endif

#if BUILDFLAG(IS_WIN)
#include <variant>
#include "shell/browser/ui/views/win_frame_view.h"
#include "shell/browser/ui/win/taskbar_host.h"
#include "ui/base/win/shell.h"
#elif BUILDFLAG(IS_LINUX)
#include "shell/browser/ui/views/electron_frame_view_linux.h"
#include "ui/gfx/image/image_skia.h"
#endif

#if BUILDFLAG(IS_WIN)
namespace gin {

template <>
struct Converter<electron::TaskbarHost::ThumbarButton> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::TaskbarHost::ThumbarButton* out) {
    gin::Dictionary dict(isolate);
    if (!gin::ConvertFromV8(isolate, val, &dict))
      return false;
    dict.Get("click", &(out->clicked_callback));
    dict.Get("tooltip", &(out->tooltip));
    dict.Get("flags", &out->flags);
    return dict.Get("icon", &(out->icon));
  }
};

}  // namespace gin
#endif

namespace electron::api {

namespace {

// Reads |key| into |out| when the dictionary has it. Returns false when it is
// there but is not a finite number that fits in an int, the same values the
// gfx::Rect converter rejects.
bool ReadCoordinate(gin_helper::Dictionary& dict,
                    std::string_view key,
                    std::optional<float>* out) {
  v8::Local<v8::Value> field;
  if (!dict.Get(key, &field) || field->IsUndefined())
    return true;
  double value = 0;
  if (!gin::ConvertFromV8(dict.isolate(), field, &value) ||
      !std::isfinite(value) || value < std::numeric_limits<int>::min() ||
      value > std::numeric_limits<int>::max())
    return false;
  *out = static_cast<float>(value);
  return true;
}

#if !BUILDFLAG(IS_MAC)
// Converts binary data to Buffer.
v8::Local<v8::Value> ToBuffer(v8::Isolate* isolate,
                              const base::span<const uint8_t> val) {
  auto buffer = electron::Buffer::Copy(isolate, val);
  if (buffer.IsEmpty())
    return v8::Null(isolate);
  else
    return buffer.ToLocalChecked();
}
#endif

[[nodiscard]] constexpr std::array<int, 2U> ToArray(const gfx::Size size) {
  return {size.width(), size.height()};
}

[[nodiscard]] constexpr std::array<int, 2U> ToArray(const gfx::Point point) {
  return {point.x(), point.y()};
}

}  // namespace

BaseWindow::BaseWindow(v8::Isolate* isolate,
                       const gin_helper::Dictionary& options) {
  // make sure we don't override title on back/forward navigation
  // if the title is provided
  if (std::string title; options.Get(options::kTitle, &title))
    title_set_from_api_ = true;

  // The parent window.
  gin_helper::Handle<BaseWindow> parent;
  if (options.Get("parent", &parent) && !parent.IsEmpty())
    parent_window_.Reset(isolate, parent.ToV8());

  // Offscreen windows are always created frameless.
  gin_helper::Dictionary web_preferences;
  bool offscreen;
  if (options.Get(options::kWebPreferences, &web_preferences) &&
      web_preferences.Get(options::kOffscreen, &offscreen) && offscreen) {
    const_cast<gin_helper::Dictionary&>(options).Set(options::kFrame, false);
  }

  // Creates NativeWindow.
  NativeWindow* parent_native = parent.IsEmpty() ? nullptr : parent->window();
  window_ = NativeWindow::Create(GetID(), options, parent_native);
  window_->AddObserver(this);

  SetContentView(View::Create(isolate));

#if defined(TOOLKIT_VIEWS)
  v8::Local<v8::Value> icon;
  if (options.Get(options::kIcon, &icon)) {
    SetIconImpl(isolate, icon, NativeImage::OnConvertError::kWarn);
  }
#endif
}

void BaseWindow::OnWrapped(v8::Isolate* isolate) {
#if !BUILDFLAG(IS_MAC)
  // The application menu is each window's menu bar until it sets its own,
  // which the JS that runs while constructing it may already do.
  if (Menu* menu = Menu::application_menu())
    SetMenuNatively(menu);
#endif
}

BaseWindow::BaseWindow(gin::Arguments* args,
                       const gin_helper::Dictionary& options)
    : BaseWindow(args->isolate(), options) {
  InitWithArgs(args);
  // Init window after everything has been setup.
  window()->InitFromOptions(options);
}

BaseWindow::~BaseWindow() {
  CloseImmediately();

  // Destroy the native window in next tick because the native code might be
  // iterating all windows.
  base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
      FROM_HERE, window_.release());

  // Remove global reference so the JS object can be garbage collected.
  self_ref_.Reset();
}

void BaseWindow::InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper) {
  gin_helper::TrackableObject<BaseWindow>::InitWith(isolate, wrapper);

  // Reference this object in case it got garbage collected.
  self_ref_.Reset(isolate, wrapper);
}

void BaseWindow::WillCloseWindow(bool* prevent_default) {
  if (Emit("close")) {
    *prevent_default = true;
  }
}

void BaseWindow::OnWindowClosed() {
  // Invalidate weak ptrs before the Javascript object is destroyed,
  // there might be some delayed emit events which shouldn't be
  // triggered after this.
  weak_factory_.InvalidateWeakPtrs();

  RemoveFromWeakMap();
  window_->RemoveObserver(this);

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();
  window_->FlushWindowState();
  Emit("closed");

  parent_window_.Reset();

  // Destroy the native class when window is closed.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, GetDestroyClosure());
}

void BaseWindow::OnWindowQueryEndSession(
    const std::vector<std::string>& reasons,
    bool* prevent_default) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);

  gin_helper::internal::Event* event =
      gin_helper::internal::Event::New(isolate);
  v8::Local<v8::Object> event_object =
      event->GetWrapper(isolate).ToLocalChecked();

  gin::Dictionary dict(isolate, event_object);
  dict.Set("reasons", reasons);

  EmitWithoutEvent("query-session-end", event_object);
  if (event->GetDefaultPrevented()) {
    *prevent_default = true;
  }
}

void BaseWindow::OnWindowEndSession(const std::vector<std::string>& reasons) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);

  gin_helper::internal::Event* event =
      gin_helper::internal::Event::New(isolate);
  v8::Local<v8::Object> event_object =
      event->GetWrapper(isolate).ToLocalChecked();

  gin::Dictionary dict(isolate, event_object);
  dict.Set("reasons", reasons);

  EmitWithoutEvent("session-end", event_object);
}

void BaseWindow::OnWindowBlur() {
  EmitEventSoon("blur");
}

void BaseWindow::OnWindowFocus() {
  EmitEventSoon("focus");
}

void BaseWindow::OnWindowShow() {
  Emit("show");
}

void BaseWindow::OnWindowHide() {
  Emit("hide");
}

void BaseWindow::OnWindowMaximize() {
  // Persist the display mode; bounds events fired during the transition
  // are skipped by SaveWindowState, so nothing else would.
  window_->DebouncedSaveWindowState();
  Emit("maximize");
}

void BaseWindow::OnWindowUnmaximize() {
  window_->DebouncedSaveWindowState();
  Emit("unmaximize");
}

void BaseWindow::OnWindowMinimize() {
  Emit("minimize");
}

void BaseWindow::OnWindowRestore() {
  Emit("restore");
}

void BaseWindow::OnWindowWillResize(const gfx::Rect& new_bounds,
                                    const gfx::ResizeEdge edge,
                                    bool* prevent_default) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto info = gin::Dictionary::CreateEmpty(isolate);
  info.Set("edge", edge);

  if (Emit("will-resize", new_bounds, info)) {
    *prevent_default = true;
  }
}

void BaseWindow::OnWindowResize() {
  window_->DebouncedSaveWindowState();
  Emit("resize");
}

void BaseWindow::OnWindowResized() {
  Emit("resized");
}

void BaseWindow::OnWindowWillMove(const gfx::Rect& new_bounds,
                                  bool* prevent_default) {
  if (Emit("will-move", new_bounds)) {
    *prevent_default = true;
  }
}

void BaseWindow::OnWindowMove() {
  window_->DebouncedSaveWindowState();
  Emit("move");
}

void BaseWindow::OnWindowMoved() {
  Emit("moved");
}

void BaseWindow::OnWindowEnterFullScreen() {
  window_->DebouncedSaveWindowState();
  Emit("enter-full-screen");
}

void BaseWindow::OnWindowLeaveFullScreen() {
  window_->DebouncedSaveWindowState();
  Emit("leave-full-screen");
}

void BaseWindow::OnWindowSwipe(const std::string& direction) {
  Emit("swipe", direction);
}

void BaseWindow::OnWindowRotateGesture(float rotation) {
  Emit("rotate-gesture", rotation);
}

void BaseWindow::OnWindowSheetBegin() {
  Emit("sheet-begin");
}

void BaseWindow::OnWindowSheetEnd() {
  Emit("sheet-end");
}

void BaseWindow::OnWindowIsKeyChanged(bool is_key) {
#if BUILDFLAG(IS_MAC)
  window()->SetActive(is_key);
#endif
}

void BaseWindow::OnWindowEnterHtmlFullScreen() {
  Emit("enter-html-full-screen");
}

void BaseWindow::OnWindowLeaveHtmlFullScreen() {
  Emit("leave-html-full-screen");
}

void BaseWindow::OnWindowAlwaysOnTopChanged(const bool is_always_on_top) {
  Emit("always-on-top-changed", is_always_on_top);
}

void BaseWindow::OnExecuteAppCommand(const std::string_view command_name) {
  Emit("app-command", command_name);
}

void BaseWindow::OnTouchBarItemResult(const std::string& item_id,
                                      const base::DictValue& details) {
  Emit("-touch-bar-interaction", item_id, details);
}

void BaseWindow::OnNewWindowForTab() {
  Emit("new-window-for-tab");
}

void BaseWindow::OnSystemContextMenu(int x, int y, bool* prevent_default) {
  if (Emit("system-context-menu", gfx::Point(x, y))) {
    *prevent_default = true;
  }
}

void BaseWindow::OnWindowStateRestored() {
  EmitEventSoon("persisted-state-restored");
}

#if BUILDFLAG(IS_WIN)
void BaseWindow::OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) {
  if (IsWindowMessageHooked(message)) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    messages_callback_map_[message].Run(
        ToBuffer(isolate, base::byte_span_from_ref(w_param)),
        ToBuffer(isolate, base::byte_span_from_ref(l_param)));
  }
}
#endif

void BaseWindow::SetContentView(View* view) {
  content_view_ = view;
  window_->SetContentView(view->view());
}

void BaseWindow::CloseImmediately() {
  if (!window_->IsClosed())
    window_->CloseImmediately();
}

void BaseWindow::Focus() {
  window_->Focus(true);
}

void BaseWindow::Blur() {
  window_->Focus(false);
}

void BaseWindow::Show() {
  window_->Show();
}

void BaseWindow::ShowInactive() {
  // This method doesn't make sense for modal window.
  if (window_->is_modal())
    return;
  window_->ShowInactive();
}

void BaseWindow::SetBounds(v8::Local<v8::Object> partial,
                           gin::Arguments* const args) {
  // Any of x, y, width and height may be left out and keeps its current
  // value. The rounding is the same as gfx::Rect's converter.
  gin_helper::Dictionary dict(args->isolate(), partial);
  std::optional<float> x, y, width, height;
  if (!ReadCoordinate(dict, "x", &x) || !ReadCoordinate(dict, "y", &y) ||
      !ReadCoordinate(dict, "width", &width) ||
      !ReadCoordinate(dict, "height", &height)) {
    args->ThrowError();
    return;
  }
  gfx::RectF bounds(window_->GetBounds());
  if (x)
    bounds.set_x(*x);
  if (y)
    bounds.set_y(*y);
  if (width)
    bounds.set_width(*width);
  if (height)
    bounds.set_height(*height);
  bool animate = false;
  args->GetNext(&animate);
  window_->SetBounds(gfx::ToRoundedRect(bounds), animate);
}

void BaseWindow::SetContentBounds(const gfx::Rect& bounds,
                                  gin::Arguments* const args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetContentBounds(bounds, animate);
}

void BaseWindow::SetSize(int width, int height, gin::Arguments* args) {
  bool animate = false;
  gfx::Size size = window_->GetMinimumSize();
  size.SetToMax(gfx::Size(width, height));
  args->GetNext(&animate);
  window_->SetSize(size, animate);
}

std::array<int, 2U> BaseWindow::GetSize() const {
  return ToArray(window_->GetSize());
}

void BaseWindow::SetContentSize(const int width,
                                const int height,
                                gin::Arguments* const args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetContentSize(gfx::Size{width, height}, animate);
}

std::array<int, 2U> BaseWindow::GetContentSize() const {
  return ToArray(window_->GetContentSize());
}

void BaseWindow::SetMinimumSize(int width, int height) {
  window_->SetMinimumSize(gfx::Size(width, height));
}

std::array<int, 2U> BaseWindow::GetMinimumSize() const {
  return ToArray(window_->GetMinimumSize());
}

void BaseWindow::SetMaximumSize(int width, int height) {
  window_->SetMaximumSize(gfx::Size(width, height));
}

std::array<int, 2U> BaseWindow::GetMaximumSize() const {
  return ToArray(window_->GetMaximumSize());
}

void BaseWindow::SetSheetOffset(const double offsetY,
                                gin::Arguments* const args) {
  double offsetX = 0.0;
  args->GetNext(&offsetX);
  window_->SetSheetOffset(offsetX, offsetY);
}

void BaseWindow::SetAlwaysOnTop(bool top, gin::Arguments* args) {
  std::string level = "floating";
  int relative_level = 0;
  args->GetNext(&level);
  args->GetNext(&relative_level);

  ui::ZOrderLevel z_order =
      top ? ui::ZOrderLevel::kFloatingWindow : ui::ZOrderLevel::kNormal;
  window_->SetAlwaysOnTop(z_order, level, relative_level);
}

bool BaseWindow::IsAlwaysOnTop() const {
  return window_->GetZOrderLevel() != ui::ZOrderLevel::kNormal;
}

void BaseWindow::SetPosition(const int x,
                             const int y,
                             gin::Arguments* const args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetPosition(gfx::Point{x, y}, animate);
}

std::array<int, 2U> BaseWindow::GetPosition() const {
  return ToArray(window_->GetPosition());
}
void BaseWindow::MoveAbove(const std::string& sourceId,
                           gin::Arguments* const args) {
  if (!window_->MoveAbove(sourceId))
    args->ThrowTypeError("Invalid media source id");
}

void BaseWindow::SetTitle(const std::string& title) {
  title_set_from_api_ = true;
  window_->SetTitle(title);
}

void BaseWindow::SetTitleFromPage(const std::string& title) {
  title_set_from_api_ = false;
  window_->SetTitle(title);
}

bool BaseWindow::SetTitleFromPageIfNotSetFromApi(const std::string& title) {
  if (title_set_from_api_) {
    return false;
  } else {
    SetTitleFromPage(title);
    return true;
  }
}

void BaseWindow::SetBackgroundColor(const std::string& color_name) {
  SkColor color = ParseCSSColor(color_name).value_or(SK_ColorWHITE);
  window_->SetBackgroundColor(color);
}

std::string BaseWindow::GetBackgroundColor() const {
  return ToRGBHex(window_->GetBackgroundColor());
}

void BaseWindow::SetIgnoreMouseEvents(bool ignore, gin::Arguments* const args) {
  gin_helper::Dictionary options;
  bool forward = false;
  args->GetNext(&options) && options.Get("forward", &forward);
  return window_->SetIgnoreMouseEvents(ignore, forward);
}

// static
BaseWindow* BaseWindow::GetFocusedWindow() {
  for (BaseWindow* window : GetAllNative()) {
    if (window->window() && window->window()->IsFocused())
      return window;
  }
  return nullptr;
}

// static
BaseWindow* BaseWindow::FromValue(v8::Isolate* isolate,
                                  v8::Local<v8::Value> value) {
  if (value.IsEmpty() || !value->IsObject())
    return nullptr;
  for (BaseWindow* window : GetAllNative()) {
    if (window->window() && window->GetWrapper() == value)
      return window;
  }
  return nullptr;
}

// static
bool BaseWindow::IsLive(const BaseWindow* window) {
  for (BaseWindow* live : GetAllNative()) {
    if (live == window)
      return live->window() != nullptr;
  }
  return false;
}

void BaseWindow::SetMenuNatively(Menu* menu) {
  // We only want to update the menu if the menu has a non-zero item count,
  // or we risk crashes.
  if (menu->model()->GetItemCount() == 0) {
    RemoveMenu();
  } else {
    window_->SetMenu(menu->model());
  }
  menu_ = menu;
}

void BaseWindow::SetMenu(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  auto context = isolate->GetCurrentContext();
  Menu* menu = nullptr;
  v8::Local<v8::Object> object;
  if (value->IsObject() && value->ToObject(context).ToLocal(&object) &&
      gin::ConvertFromV8(isolate, value, &menu) && menu) {
    SetMenuNatively(menu);
  } else if (value->IsNull()) {
    RemoveMenu();
  } else {
    isolate->ThrowException(
        v8::Exception::TypeError(gin::StringToV8(isolate, "Invalid Menu")));
  }
}

void BaseWindow::RemoveMenu() {
  menu_.Clear();
  window_->SetMenu(nullptr);
}

void BaseWindow::SetParentWindow(v8::Local<v8::Value> value,
                                 gin::Arguments* const args) {
  if (window_->is_modal()) {
    args->ThrowTypeError("Can not be called for modal window");
    return;
  }

  gin_helper::Handle<BaseWindow> parent;
  if (value->IsNull() || value->IsUndefined()) {
    parent_window_.Reset();
    window_->SetParentWindow(nullptr);
  } else if (window_->IsInAppShim()) {
    args->ThrowTypeError("A window in an app shim can't have a parent window");
  } else if (gin::ConvertFromV8(isolate(), value, &parent)) {
    if (parent->window() && parent->window()->IsInAppShim()) {
      args->ThrowTypeError("A window in an app shim can't be a parent window");
      return;
    }
    parent_window_.Reset(isolate(), parent.ToV8());
    window_->SetParentWindow(parent->window());
  } else {
    args->ThrowTypeError("Must pass BaseWindow instance or null");
  }
}

std::string BaseWindow::GetMediaSourceId() const {
  return window_->GetDesktopMediaID().ToString();
}

#if !BUILDFLAG(IS_MAC)
v8::Local<v8::Value> BaseWindow::GetNativeWindowHandle() {
  // TODO(MarshallOfSound): Replace once
  // https://chromium-review.googlesource.com/c/chromium/src/+/1253094/ has
  // landed
  NativeWindowHandle handle = window_->GetNativeWindowHandle();
  return ToBuffer(isolate(), base::byte_span_from_ref(handle));
}
#endif

void BaseWindow::SetProgressBar(double progress, gin::Arguments* args) {
  gin_helper::Dictionary options;
  std::string mode;
  args->GetNext(&options) && options.Get("mode", &mode);

  NativeWindow::ProgressState state = NativeWindow::ProgressState::kNormal;
  if (mode == "error")
    state = NativeWindow::ProgressState::kError;
  else if (mode == "paused")
    state = NativeWindow::ProgressState::kPaused;
  else if (mode == "indeterminate")
    state = NativeWindow::ProgressState::kIndeterminate;
  else if (mode == "none")
    state = NativeWindow::ProgressState::kNone;

  window_->SetProgressBar(progress, state);
}

void BaseWindow::SetVisibleOnAllWorkspaces(bool visible,
                                           gin::Arguments* const args) {
  gin_helper::Dictionary options;
  bool visibleOnFullScreen = false;
  bool skipTransformProcessType = false;
  if (args->GetNext(&options)) {
    options.Get("visibleOnFullScreen", &visibleOnFullScreen);
    options.Get("skipTransformProcessType", &skipTransformProcessType);
  }
  return window_->SetVisibleOnAllWorkspaces(visible, visibleOnFullScreen,
                                            skipTransformProcessType);
}

void BaseWindow::SetVibrancy(v8::Isolate* const isolate,
                             v8::Local<v8::Value> value,
                             gin::Arguments* const args) {
  std::string type = gin::V8ToString(isolate, value);
  gin_helper::Dictionary options;
  int animation_duration_ms = 0;

  if (args->GetNext(&options)) {
    options.Get("animationDuration", &animation_duration_ms);
  }

  window_->SetVibrancy(type, animation_duration_ms);
}

void BaseWindow::SetBackgroundMaterial(const std::string& material) {
  window_->SetBackgroundMaterial(material);
}

void BaseWindow::AddTabbedWindow(NativeWindow* const window,
                                 gin::Arguments* const args) {
  if (!window_->AddTabbedWindow(window)) {
    args->ThrowTypeError(
        window_.get() != window &&
                (window_->IsInAppShim() || window->IsInAppShim())
            ? "AddTabbedWindow needs both windows to be in the same app"
            : "AddTabbedWindow cannot be called by a window on itself.");
  }
}

v8::Local<v8::Value> BaseWindow::GetTabbingIdentifier() {
  auto tabbing_id = window_->GetTabbingIdentifier();
  if (!tabbing_id.has_value())
    return v8::Undefined(isolate());

  return gin::ConvertToV8(isolate(), tabbing_id.value());
}

void BaseWindow::SetAspectRatio(const double aspect_ratio,
                                gin::Arguments* const args) {
  gfx::Size extra_size;
  args->GetNext(&extra_size);
  window_->SetAspectRatio(aspect_ratio, extra_size);
}

void BaseWindow::PreviewFile(const std::string& path,
                             gin::Arguments* const args) {
  std::string display_name;
  if (!args->GetNext(&display_name))
    display_name = path;
  window_->PreviewFile(path, display_name);
}

v8::Local<v8::Value> BaseWindow::GetContentView() const {
  v8::Local<v8::Object> wrapper;
  if (content_view_ && content_view_->GetWrapper(isolate()).ToLocal(&wrapper))
    return wrapper;
  return v8::Null(isolate());
}

BaseWindow* BaseWindow::GetParentWindow() const {
  if (parent_window_.IsEmpty())
    return nullptr;

  v8::HandleScope scope{isolate()};
  auto local = v8::Local<v8::Value>::New(isolate(), parent_window_);
  BaseWindow* parent = nullptr;
  gin::ConvertFromV8(isolate(), local, &parent);
  return parent;
}

std::vector<BaseWindow*> BaseWindow::GetChildWindows() const {
  std::vector<BaseWindow*> children;
  auto* const isolate = this->isolate();
  v8::HandleScope scope{isolate};
  for (auto wrapper : BaseWindow::GetAll(isolate)) {
    BaseWindow* win = nullptr;
    gin::ConvertFromV8(isolate, wrapper, &win);
    if (win && win->GetParentWindow() == this)
      children.emplace_back(win);
  }
  return children;
}

bool BaseWindow::SetThumbarButtons(gin::Arguments* args) {
#if BUILDFLAG(IS_WIN)
  std::vector<TaskbarHost::ThumbarButton> buttons;
  if (!args->GetNext(&buttons)) {
    args->ThrowError();
    return false;
  }
  auto* window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbarButtons(
      window_->GetAcceleratedWidget(), buttons);
#else
  return false;
#endif
}

#if defined(TOOLKIT_VIEWS)
void BaseWindow::SetIcon(v8::Isolate* isolate, v8::Local<v8::Value> icon) {
  SetIconImpl(isolate, icon, NativeImage::OnConvertError::kThrow);
}

void BaseWindow::SetIconImpl(v8::Isolate* isolate,
                             v8::Local<v8::Value> icon,
                             NativeImage::OnConvertError on_error) {
  NativeImage* native_image = nullptr;
  if (!NativeImage::TryConvertNativeImage(isolate, icon, &native_image,
                                          on_error))
    return;

#if BUILDFLAG(IS_WIN)
  static_cast<NativeWindowViews*>(window_.get())
      ->SetIcon(native_image->GetHICON(GetSystemMetrics(SM_CXSMICON)),
                native_image->GetHICON(GetSystemMetrics(SM_CXICON)));
#elif BUILDFLAG(IS_LINUX)
  static_cast<NativeWindowViews*>(window_.get())
      ->SetIcon(native_image->image().AsImageSkia());
#endif
}
#endif

#if BUILDFLAG(IS_WIN)
bool BaseWindow::HookWindowMessage(UINT message,
                                   const MessageCallback& callback) {
  messages_callback_map_[message] = callback;
  return true;
}

void BaseWindow::UnhookWindowMessage(UINT message) {
  messages_callback_map_.erase(message);
}

bool BaseWindow::IsWindowMessageHooked(UINT message) {
  return messages_callback_map_.contains(message);
}

void BaseWindow::UnhookAllWindowMessages() {
  messages_callback_map_.clear();
}

bool BaseWindow::SetThumbnailClip(const gfx::Rect& region) {
  auto* window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbnailClip(
      window_->GetAcceleratedWidget(), region);
}

bool BaseWindow::SetThumbnailToolTip(const std::string& tooltip) {
  auto* window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbnailToolTip(
      window_->GetAcceleratedWidget(), tooltip);
}

void BaseWindow::SetAppDetails(const gin_helper::Dictionary& options) {
  std::wstring app_id;
  base::FilePath app_icon_path;
  int app_icon_index = 0;
  std::wstring relaunch_command;
  std::wstring relaunch_display_name;

  options.Get("appId", &app_id);
  options.Get("appIconPath", &app_icon_path);
  options.Get("appIconIndex", &app_icon_index);
  options.Get("relaunchCommand", &relaunch_command);
  options.Get("relaunchDisplayName", &relaunch_display_name);

  ui::win::SetAppDetailsForWindow(app_id, app_icon_path, app_icon_index,
                                  relaunch_command, relaunch_display_name,
                                  window_->GetAcceleratedWidget());
}

void BaseWindow::SetAccentColor(gin::Arguments* const args) {
  v8::Local<v8::Value> ac_val;
  args->GetNext(&ac_val);

  if (!ac_val.IsEmpty() && ac_val->IsNull()) {
    window_->SetAccentColor(std::monostate{});
    window_->UpdateWindowAccentColor(window_->IsActive());
    return;
  }

  if (!ac_val.IsEmpty() && ac_val->IsBoolean()) {
    const bool ac_flag = ac_val->BooleanValue(args->isolate());
    window_->SetAccentColor(ac_flag);
    window_->UpdateWindowAccentColor(window_->IsActive());
    return;
  }

  if (!ac_val.IsEmpty() && ac_val->IsString()) {
    std::string ac_str;
    gin::ConvertFromV8(args->isolate(), ac_val, &ac_str);
    if (const std::optional<SkColor> ac_color = ParseCSSColor(ac_str)) {
      window_->SetAccentColor(*ac_color);
      window_->UpdateWindowAccentColor(window_->IsActive());
    }
    return;
  }

  args->ThrowTypeError(
      "Invalid accent color value - must be null, hex string, or boolean");
}

v8::Local<v8::Value> BaseWindow::GetAccentColor() const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  auto accent_color = window_->GetAccentColor();

  if (std::holds_alternative<bool>(accent_color))
    return v8::Boolean::New(isolate, std::get<bool>(accent_color));
  return gin::StringToV8(isolate, std::get<std::string>(accent_color));
}
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
void BaseWindow::SetTitleBarOverlay(const gin_helper::Dictionary& options,
                                    gin::Arguments* args) {
  static_cast<NativeWindowViews*>(window_.get())
      ->SetTitleBarOverlay(options, args);
}
#endif

// static
void BaseWindow::ClearPersistedState(const std::string& window_name) {
  if (window_name.empty()) {
    LOG(WARNING) << "Cannot clear persisted window state: window name is empty";
    return;
  }

  if (auto* browser_process =
          electron::ElectronBrowserMainParts::Get()->browser_process()) {
    DCHECK(browser_process);
    if (auto* prefs = browser_process->local_state()) {
      ScopedDictPrefUpdate update(prefs, electron::kWindowStates);

      if (!update->Remove(window_name)) {
        LOG(WARNING) << "Window state '" << window_name
                     << "' not found, nothing to clear";
      }
    }
  }
}

// static
gin_helper::WrappableBase* BaseWindow::New(gin::Arguments* const args) {
  auto options = gin_helper::Dictionary::CreateEmpty(args->isolate());
  args->GetNext(&options);

  std::string error_message;
  if (!IsWindowNameValid(options, &error_message)) {
    // Window name is already in use throw an error and do not create the window
    args->ThrowTypeError(error_message);
    return nullptr;
  }
  if (!IsAppShimValid(options, &error_message) ||
      !IsParentValid(options, &error_message)) {
    args->ThrowTypeError(error_message);
    return nullptr;
  }

  return new BaseWindow(args, options);
}

// static
bool BaseWindow::IsWindowNameValid(const gin_helper::Dictionary& options,
                                   std::string* error_message) {
  std::string window_name;
  if (options.Get(options::kName, &window_name) && !window_name.empty()) {
    // Check if window name is already in use by another window
    // Window names must be unique for state persistence to work correctly
    const auto& windows = electron::WindowList::GetWindows();
    bool name_in_use = std::any_of(windows.begin(), windows.end(),
                                   [&window_name](const auto* const window) {
                                     return window->GetName() == window_name;
                                   });

    if (name_in_use) {
      *error_message = "Window name '" + window_name +
                       "' is already in use. Window names must be unique.";
      return false;
    }
  }
  return true;
}

// static
bool BaseWindow::IsAppShimValid(const gin_helper::Dictionary& options,
                                std::string* error_message) {
#if BUILDFLAG(IS_MAC)
  v8::Local<v8::Value> value;
  if (!options.Get(options::kAppShim, &value) || value->IsNullOrUndefined())
    return true;
  AppShimHost* app_shim = nullptr;
  if (!gin::ConvertFromV8(options.isolate(), value, &app_shim) || !app_shim) {
    *error_message = "appShim must be an AppShimHost";
    return false;
  }
  if (!app_shim->GetRemoteCocoaApplicationHost()) {
    *error_message = "The app shim has disconnected";
    return false;
  }
  if (bool modal = false; options.Get(options::kModal, &modal) && modal) {
    *error_message = "A window in an app shim can't be modal";
    return false;
  }
  if (v8::Local<v8::Value> parent;
      options.Get("parent", &parent) && !parent->IsNullOrUndefined()) {
    *error_message = "A window in an app shim can't have a parent window";
    return false;
  }
#endif
  return true;
}

// static
bool BaseWindow::IsParentValid(const gin_helper::Dictionary& options,
                               std::string* error_message) {
  gin_helper::Handle<BaseWindow> parent;
  if (options.Get("parent", &parent) && !parent.IsEmpty() && parent->window() &&
      parent->window()->IsInAppShim()) {
    *error_message = "A window in an app shim can't be a parent window";
    return false;
  }
  return true;
}

// static
v8::Local<v8::FunctionTemplate> BaseWindow::GetConstructorTemplate(
    v8::Isolate* isolate) {
  static bool created = false;
  if (!created) {
    created = true;
    SetConstructor(isolate, base::BindRepeating(&BaseWindow::New));
  }
  return GetConstructor(isolate);
}

// static
void BaseWindow::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "BaseWindow"));
  prototype->Inherit(gin_helper::internal::GetEventEmitterTemplate(isolate));
  gin_helper::Destroyable::MakeDestroyable(isolate, prototype);
  // A NativeWindow method bound here takes its receiver from the JavaScript
  // `this` through gin::Converter<NativeWindow*>, which unwraps the BaseWindow
  // wrapper, so calling one on a destroyed window still throws "Object has
  // been destroyed".
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod<&BaseWindow::SetContentView>("setContentView")
      .SetMethod<&NativeWindow::Close>("close")
      .SetMethod<&BaseWindow::Focus>("focus")
      .SetMethod<&BaseWindow::Blur>("blur")
      .SetMethod<&NativeWindow::IsFocused>("isFocused")
      .SetMethod<&BaseWindow::Show>("show")
      .SetMethod<&BaseWindow::ShowInactive>("showInactive")
      .SetMethod<&NativeWindow::Hide>("hide")
      .SetMethod<&NativeWindow::IsVisible>("isVisible")
      .SetMethod<&NativeWindow::IsEnabled>("isEnabled")
      .SetMethod<&NativeWindow::SetEnabled>("setEnabled")
      .SetMethod<&NativeWindow::Maximize>("maximize")
      .SetMethod<&NativeWindow::Unmaximize>("unmaximize")
      .SetMethod<&NativeWindow::IsMaximized>("isMaximized")
      .SetMethod<&NativeWindow::Minimize>("minimize")
      .SetMethod<&NativeWindow::Restore>("restore")
      .SetMethod<&NativeWindow::IsMinimized>("isMinimized")
      .SetMethod<&NativeWindow::SetFullScreen>("setFullScreen")
      .SetMethod<&NativeWindow::IsFullscreen>("isFullScreen")
      .SetProperty<&NativeWindow::IsFullscreen, &NativeWindow::SetFullScreen>(
          "fullScreen")
      .SetMethod<&BaseWindow::SetBounds>("setBounds")
      .SetMethod<&NativeWindow::GetBounds>("getBounds")
      .SetMethod<&NativeWindow::IsNormal>("isNormal")
      .SetMethod<&NativeWindow::GetNormalBounds>("getNormalBounds")
      .SetMethod<&BaseWindow::SetSize>("setSize")
      .SetMethod<&BaseWindow::GetSize>("getSize")
      .SetMethod<&BaseWindow::SetContentBounds>("setContentBounds")
      .SetMethod<&NativeWindow::GetContentBounds>("getContentBounds")
      .SetMethod<&BaseWindow::SetContentSize>("setContentSize")
      .SetMethod<&BaseWindow::GetContentSize>("getContentSize")
      .SetMethod<&BaseWindow::SetMinimumSize>("setMinimumSize")
      .SetMethod<&BaseWindow::GetMinimumSize>("getMinimumSize")
      .SetMethod<&BaseWindow::SetMaximumSize>("setMaximumSize")
      .SetMethod<&BaseWindow::GetMaximumSize>("getMaximumSize")
      .SetMethod<&BaseWindow::SetSheetOffset>("setSheetOffset")
      .SetMethod<&BaseWindow::MoveAbove>("moveAbove")
      .SetMethod<&NativeWindow::MoveTop>("moveTop")
      .SetMethod<&NativeWindow::SetResizable>("setResizable")
      .SetMethod<&NativeWindow::IsResizable>("isResizable")
      .SetProperty<&NativeWindow::IsResizable, &NativeWindow::SetResizable>(
          "resizable")
      .SetMethod<&NativeWindow::SetMovable>("setMovable")
      .SetMethod<&NativeWindow::IsMovable>("isMovable")
      .SetProperty<&NativeWindow::IsMovable, &NativeWindow::SetMovable>(
          "movable")
      .SetMethod<&NativeWindow::SetMinimizable>("setMinimizable")
      .SetMethod<&NativeWindow::IsMinimizable>("isMinimizable")
      .SetProperty<&NativeWindow::IsMinimizable, &NativeWindow::SetMinimizable>(
          "minimizable")
      .SetMethod<&NativeWindow::SetMaximizable>("setMaximizable")
      .SetMethod<&NativeWindow::IsMaximizable>("isMaximizable")
      .SetProperty<&NativeWindow::IsMaximizable, &NativeWindow::SetMaximizable>(
          "maximizable")
      .SetMethod<&NativeWindow::SetFullScreenable>("setFullScreenable")
      .SetMethod<&NativeWindow::IsFullScreenable>("isFullScreenable")
      .SetProperty<&NativeWindow::IsFullScreenable,
                   &NativeWindow::SetFullScreenable>("fullScreenable")
      .SetMethod<&NativeWindow::SetClosable>("setClosable")
      .SetMethod<&NativeWindow::IsClosable>("isClosable")
      .SetProperty<&NativeWindow::IsClosable, &NativeWindow::SetClosable>(
          "closable")
      .SetMethod<&BaseWindow::SetAlwaysOnTop>("setAlwaysOnTop")
      .SetMethod<&BaseWindow::IsAlwaysOnTop>("isAlwaysOnTop")
      .SetMethod<&NativeWindow::Center>("center")
      .SetMethod<&BaseWindow::SetPosition>("setPosition")
      .SetMethod<&BaseWindow::GetPosition>("getPosition")
      .SetMethod<&BaseWindow::SetTitle>("setTitle")
      .SetMethod<&NativeWindow::GetTitle>("getTitle")
      .SetProperty<&NativeWindow::GetTitle, &BaseWindow::SetTitle>("title")
      .SetProperty<&NativeWindow::GetAccessibleTitle,
                   &NativeWindow::SetAccessibleTitle>("accessibleTitle")
      .SetMethod<&NativeWindow::FlashFrame>("flashFrame")
      .SetMethod<&NativeWindow::SetSkipTaskbar>("setSkipTaskbar")
      .SetMethod<&NativeWindow::SetSimpleFullScreen>("setSimpleFullScreen")
      .SetMethod<&NativeWindow::IsSimpleFullScreen>("isSimpleFullScreen")
      .SetProperty<&NativeWindow::IsSimpleFullScreen,
                   &NativeWindow::SetSimpleFullScreen>("simpleFullScreen")
      .SetMethod<&NativeWindow::SetKiosk>("setKiosk")
      .SetMethod<&NativeWindow::IsKiosk>("isKiosk")
      .SetProperty<&NativeWindow::IsKiosk, &NativeWindow::SetKiosk>("kiosk")
      .SetMethod<&NativeWindow::IsTabletMode>("isTabletMode")
      .SetMethod<&BaseWindow::SetBackgroundColor>("setBackgroundColor")
      .SetMethod<&BaseWindow::GetBackgroundColor>("getBackgroundColor")
      .SetMethod<&NativeWindow::SetHasShadow>("setHasShadow")
      .SetMethod<&NativeWindow::HasShadow>("hasShadow")
      .SetProperty<&NativeWindow::HasShadow, &NativeWindow::SetHasShadow>(
          "shadow")
      .SetMethod<&NativeWindow::SetOpacity>("setOpacity")
      .SetMethod<&NativeWindow::GetOpacity>("getOpacity")
      .SetMethod<&NativeWindow::SetShape>("setShape")
      .SetMethod<&NativeWindow::SetRepresentedFilename>(
          "setRepresentedFilename")
      .SetMethod<&NativeWindow::GetRepresentedFilename>(
          "getRepresentedFilename")
      .SetProperty<&NativeWindow::GetRepresentedFilename,
                   &NativeWindow::SetRepresentedFilename>("representedFilename")
      .SetMethod<&NativeWindow::SetDocumentEdited>("setDocumentEdited")
      .SetMethod<&NativeWindow::IsDocumentEdited>("isDocumentEdited")
      .SetProperty<&NativeWindow::IsDocumentEdited,
                   &NativeWindow::SetDocumentEdited>("documentEdited")
      .SetMethod<&BaseWindow::SetIgnoreMouseEvents>("setIgnoreMouseEvents")
      .SetMethod<&NativeWindow::SetContentProtection>("setContentProtection")
      .SetMethod<&NativeWindow::IsContentProtected>("isContentProtected")
      .SetMethod<&NativeWindow::SetFocusable>("setFocusable")
      .SetMethod<&NativeWindow::IsFocusable>("isFocusable")
      .SetProperty<&NativeWindow::IsFocusable, &NativeWindow::SetFocusable>(
          "focusable")
      .SetMethod<&BaseWindow::SetMenu>("setMenu")
      .SetMethod<&BaseWindow::RemoveMenu>("removeMenu")
      .SetMethod<&BaseWindow::SetParentWindow>("setParentWindow")
      .SetMethod<&BaseWindow::GetMediaSourceId>("getMediaSourceId")
      .SetMethod<&BaseWindow::GetNativeWindowHandle>("getNativeWindowHandle")
      .SetMethod<&BaseWindow::SetProgressBar>("setProgressBar")
      .SetMethod<&NativeWindow::SetOverlayIcon>("setOverlayIcon")
      .SetMethod<&BaseWindow::SetVisibleOnAllWorkspaces>(
          "setVisibleOnAllWorkspaces")
      .SetMethod<&NativeWindow::IsVisibleOnAllWorkspaces>(
          "isVisibleOnAllWorkspaces")
      .SetProperty<&NativeWindow::IsVisibleOnAllWorkspaces,
                   &BaseWindow::SetVisibleOnAllWorkspaces>(
          "visibleOnAllWorkspaces")
#if BUILDFLAG(IS_MAC)
      .SetMethod<&NativeWindow::InvalidateShadow>("invalidateShadow")
      .SetMethod<&NativeWindow::GetAlwaysOnTopLevel>("_getAlwaysOnTopLevel")
      .SetMethod<&NativeWindow::SetAutoHideCursor>("setAutoHideCursor")
#endif
      .SetMethod<&BaseWindow::SetVibrancy>("setVibrancy")
      .SetMethod<&BaseWindow::SetBackgroundMaterial>("setBackgroundMaterial")

#if BUILDFLAG(IS_MAC)
      .SetMethod<&NativeWindow::IsHiddenInMissionControl>(
          "isHiddenInMissionControl")
      .SetMethod<&NativeWindow::SetHiddenInMissionControl>(
          "setHiddenInMissionControl")
#endif
      .SetMethod<&NativeWindow::SetTouchBar>("_setTouchBarItems")
      .SetMethod<&NativeWindow::RefreshTouchBarItem>("_refreshTouchBarItem")
      .SetMethod<&NativeWindow::SetEscapeTouchBarItem>("_setEscapeTouchBarItem")
#if BUILDFLAG(IS_MAC)
      .SetMethod<&NativeWindow::SelectPreviousTab>("selectPreviousTab")
      .SetMethod<&NativeWindow::SelectNextTab>("selectNextTab")
      .SetMethod<&NativeWindow::ShowAllTabs>("showAllTabs")
      .SetMethod<&NativeWindow::MergeAllWindows>("mergeAllWindows")
      .SetMethod<&NativeWindow::MoveTabToNewWindow>("moveTabToNewWindow")
      .SetMethod<&NativeWindow::ToggleTabBar>("toggleTabBar")
      .SetMethod<&BaseWindow::AddTabbedWindow>("addTabbedWindow")
      .SetProperty<&BaseWindow::GetTabbingIdentifier>("tabbingIdentifier")
      .SetMethod<&NativeWindow::SetWindowButtonVisibility>(
          "setWindowButtonVisibility")
      .SetMethod<&NativeWindow::GetWindowButtonVisibility>(
          "_getWindowButtonVisibility")
      .SetMethod<&NativeWindow::SetWindowButtonPosition>(
          "setWindowButtonPosition")
      .SetMethod<&NativeWindow::GetWindowButtonPosition>(
          "getWindowButtonPosition")
      .SetProperty<&NativeWindow::IsExcludedFromShownWindowsMenu,
                   &NativeWindow::SetExcludedFromShownWindowsMenu>(
          "excludedFromShownWindowsMenu")
#endif
      .SetMethod<&NativeWindow::SetAutoHideMenuBar>("setAutoHideMenuBar")
      .SetMethod<&NativeWindow::IsMenuBarAutoHide>("isMenuBarAutoHide")
      .SetProperty<&NativeWindow::IsMenuBarAutoHide,
                   &NativeWindow::SetAutoHideMenuBar>("autoHideMenuBar")
      .SetMethod<&NativeWindow::SetMenuBarVisibility>("setMenuBarVisibility")
      .SetMethod<&NativeWindow::IsMenuBarVisible>("isMenuBarVisible")
      .SetProperty<&NativeWindow::IsMenuBarVisible,
                   &NativeWindow::SetMenuBarVisibility>("menuBarVisible")
      .SetMethod<&BaseWindow::SetAspectRatio>("setAspectRatio")
      .SetMethod<&BaseWindow::PreviewFile>("previewFile")
      .SetMethod<&NativeWindow::CloseFilePreview>("closeFilePreview")
      .SetMethod<&BaseWindow::GetContentView>("getContentView")
      .SetProperty<&BaseWindow::GetContentView, &BaseWindow::SetContentView>(
          "contentView")
      .SetMethod<&BaseWindow::GetParentWindow>("getParentWindow")
      .SetMethod<&BaseWindow::GetChildWindows>("getChildWindows")
      .SetMethod<&NativeWindow::is_modal>("isModal")
      .SetMethod<&BaseWindow::SetThumbarButtons>("setThumbarButtons")
#if defined(TOOLKIT_VIEWS)
      .SetMethod<&BaseWindow::SetIcon>("setIcon")
#endif
#if BUILDFLAG(IS_WIN)
      .SetMethod<&NativeWindow::IsSnapped>("isSnapped")
      .SetProperty<&NativeWindow::IsSnapped>("snapped")
      .SetMethod<&BaseWindow::SetAccentColor>("setAccentColor")
      .SetMethod<&BaseWindow::GetAccentColor>("getAccentColor")
      .SetMethod<&BaseWindow::HookWindowMessage>("hookWindowMessage")
      .SetMethod<&BaseWindow::IsWindowMessageHooked>("isWindowMessageHooked")
      .SetMethod<&BaseWindow::UnhookWindowMessage>("unhookWindowMessage")
      .SetMethod<&BaseWindow::UnhookAllWindowMessages>(
          "unhookAllWindowMessages")
      .SetMethod<&BaseWindow::SetThumbnailClip>("setThumbnailClip")
      .SetMethod<&BaseWindow::SetThumbnailToolTip>("setThumbnailToolTip")
      .SetMethod<&BaseWindow::SetAppDetails>("setAppDetails")
#endif
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
      .SetMethod<&BaseWindow::SetTitleBarOverlay>("setTitleBarOverlay")
#endif
      .SetProperty<&BaseWindow::GetID>("id");
}

}  // namespace electron::api

namespace {

using electron::api::BaseWindow;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary constructor(isolate,
                                     BaseWindow::GetConstructorTemplate(isolate)
                                         ->GetFunction(context)
                                         .ToLocalChecked());
  constructor.SetMethod<&BaseWindow::FromWeakMapID>("fromId");
  constructor.SetMethod<&BaseWindow::GetAll>("getAllWindows");
  constructor.SetMethod<&BaseWindow::GetFocusedWindow>("getFocusedWindow");
  constructor.SetMethod<&BaseWindow::ClearPersistedState>(
      "clearPersistedState");

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("BaseWindow", constructor);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_base_window, Initialize)
