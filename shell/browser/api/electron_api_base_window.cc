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

void BaseWindow::SetContentView(gin_helper::Handle<View> view) {
  content_view_.Reset(JavascriptEnvironment::GetIsolate(), view.ToV8());
  window_->SetContentView(view->view());
}

void BaseWindow::CloseImmediately() {
  if (!window_->IsClosed())
    window_->CloseImmediately();
}

void BaseWindow::Close() {
  window_->Close();
}

void BaseWindow::Focus() {
  window_->Focus(true);
}

void BaseWindow::Blur() {
  window_->Focus(false);
}

bool BaseWindow::IsFocused() const {
  return window_->IsFocused();
}

void BaseWindow::Show() {
  window_->Show();
}

void BaseWindow::ShowInactive() {
  // This method doesn't make sense for modal window.
  if (IsModal())
    return;
  window_->ShowInactive();
}

void BaseWindow::Hide() {
  window_->Hide();
}

bool BaseWindow::IsVisible() const {
  return window_->IsVisible();
}

bool BaseWindow::IsEnabled() const {
  return window_->IsEnabled();
}

void BaseWindow::SetEnabled(bool enable) {
  window_->SetEnabled(enable);
}

void BaseWindow::Maximize() {
  window_->Maximize();
}

void BaseWindow::Unmaximize() {
  window_->Unmaximize();
}

bool BaseWindow::IsMaximized() const {
  return window_->IsMaximized();
}

void BaseWindow::Minimize() {
  window_->Minimize();
}

void BaseWindow::Restore() {
  window_->Restore();
}

bool BaseWindow::IsMinimized() const {
  return window_->IsMinimized();
}

void BaseWindow::SetFullScreen(bool fullscreen) {
  window_->SetFullScreen(fullscreen);
}

bool BaseWindow::IsFullscreen() const {
  return window_->IsFullscreen();
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

gfx::Rect BaseWindow::GetBounds() const {
  return window_->GetBounds();
}

bool BaseWindow::IsNormal() const {
  return window_->IsNormal();
}

gfx::Rect BaseWindow::GetNormalBounds() const {
  return window_->GetNormalBounds();
}

void BaseWindow::SetContentBounds(const gfx::Rect& bounds,
                                  gin::Arguments* const args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetContentBounds(bounds, animate);
}

gfx::Rect BaseWindow::GetContentBounds() const {
  return window_->GetContentBounds();
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

void BaseWindow::SetResizable(bool resizable) {
  window_->SetResizable(resizable);
}

bool BaseWindow::IsResizable() const {
  return window_->IsResizable();
}

void BaseWindow::SetMovable(bool movable) {
  window_->SetMovable(movable);
}

bool BaseWindow::IsMovable() const {
  return window_->IsMovable();
}

void BaseWindow::SetMinimizable(bool minimizable) {
  window_->SetMinimizable(minimizable);
}

bool BaseWindow::IsMinimizable() const {
  return window_->IsMinimizable();
}

void BaseWindow::SetMaximizable(bool maximizable) {
  window_->SetMaximizable(maximizable);
}

bool BaseWindow::IsMaximizable() const {
  return window_->IsMaximizable();
}

void BaseWindow::SetFullScreenable(bool fullscreenable) {
  window_->SetFullScreenable(fullscreenable);
}

bool BaseWindow::IsFullScreenable() const {
  return window_->IsFullScreenable();
}

void BaseWindow::SetClosable(bool closable) {
  window_->SetClosable(closable);
}

bool BaseWindow::IsClosable() const {
  return window_->IsClosable();
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

void BaseWindow::Center() {
  window_->Center();
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

void BaseWindow::MoveTop() {
  window_->MoveTop();
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

std::string BaseWindow::GetTitle() const {
  return window_->GetTitle();
}

void BaseWindow::SetAccessibleTitle(const std::string& title) {
  window_->SetAccessibleTitle(title);
}

std::string BaseWindow::GetAccessibleTitle() const {
  return window_->GetAccessibleTitle();
}

void BaseWindow::FlashFrame(bool flash) {
  window_->FlashFrame(flash);
}

void BaseWindow::SetSkipTaskbar(bool skip) {
  window_->SetSkipTaskbar(skip);
}

void BaseWindow::SetExcludedFromShownWindowsMenu(bool excluded) {
  window_->SetExcludedFromShownWindowsMenu(excluded);
}

bool BaseWindow::IsExcludedFromShownWindowsMenu() const {
  return window_->IsExcludedFromShownWindowsMenu();
}

void BaseWindow::SetSimpleFullScreen(bool simple_fullscreen) {
  window_->SetSimpleFullScreen(simple_fullscreen);
}

bool BaseWindow::IsSimpleFullScreen() const {
  return window_->IsSimpleFullScreen();
}

void BaseWindow::SetKiosk(bool kiosk) {
  window_->SetKiosk(kiosk);
}

bool BaseWindow::IsKiosk() const {
  return window_->IsKiosk();
}

bool BaseWindow::IsTabletMode() const {
  return window_->IsTabletMode();
}

void BaseWindow::SetBackgroundColor(const std::string& color_name) {
  SkColor color = ParseCSSColor(color_name).value_or(SK_ColorWHITE);
  window_->SetBackgroundColor(color);
}

std::string BaseWindow::GetBackgroundColor() const {
  return ToRGBHex(window_->GetBackgroundColor());
}

void BaseWindow::InvalidateShadow() {
  window_->InvalidateShadow();
}

void BaseWindow::SetHasShadow(bool has_shadow) {
  window_->SetHasShadow(has_shadow);
}

bool BaseWindow::HasShadow() const {
  return window_->HasShadow();
}

void BaseWindow::SetOpacity(const double opacity) {
  window_->SetOpacity(opacity);
}

double BaseWindow::GetOpacity() const {
  return window_->GetOpacity();
}

void BaseWindow::SetShape(const std::vector<gfx::Rect>& rects) {
  window_->SetShape(rects);
}

void BaseWindow::SetRepresentedFilename(const std::string& filename) {
  window_->SetRepresentedFilename(filename);
}

std::string BaseWindow::GetRepresentedFilename() const {
  return window_->GetRepresentedFilename();
}

void BaseWindow::SetDocumentEdited(bool edited) {
  window_->SetDocumentEdited(edited);
}

bool BaseWindow::IsDocumentEdited() const {
  return window_->IsDocumentEdited();
}

void BaseWindow::SetIgnoreMouseEvents(bool ignore, gin::Arguments* const args) {
  gin_helper::Dictionary options;
  bool forward = false;
  args->GetNext(&options) && options.Get("forward", &forward);
  return window_->SetIgnoreMouseEvents(ignore, forward);
}

void BaseWindow::SetContentProtection(bool enable) {
  return window_->SetContentProtection(enable);
}

bool BaseWindow::IsContentProtected() const {
  return window_->IsContentProtected();
}

void BaseWindow::SetFocusable(bool focusable) {
  return window_->SetFocusable(focusable);
}

bool BaseWindow::IsFocusable() const {
  return window_->IsFocusable();
}

// static
BaseWindow* BaseWindow::GetFocusedWindow() {
  for (BaseWindow* window : GetAllNative()) {
    if (window->window() && window->IsFocused())
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
  if (IsModal()) {
    args->ThrowTypeError("Can not be called for modal window");
    return;
  }

  gin_helper::Handle<BaseWindow> parent;
  if (value->IsNull() || value->IsUndefined()) {
    parent_window_.Reset();
    window_->SetParentWindow(nullptr);
  } else if (gin::ConvertFromV8(isolate(), value, &parent)) {
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

void BaseWindow::SetOverlayIcon(const gfx::Image& overlay,
                                const std::string& description) {
  window_->SetOverlayIcon(overlay, description);
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

bool BaseWindow::IsVisibleOnAllWorkspaces() const {
  return window_->IsVisibleOnAllWorkspaces();
}

void BaseWindow::SetAutoHideCursor(bool auto_hide) {
  window_->SetAutoHideCursor(auto_hide);
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

#if BUILDFLAG(IS_MAC)
std::string BaseWindow::GetAlwaysOnTopLevel() const {
  return window_->GetAlwaysOnTopLevel();
}

void BaseWindow::SetWindowButtonVisibility(bool visible) {
  window_->SetWindowButtonVisibility(visible);
}

bool BaseWindow::GetWindowButtonVisibility() const {
  return window_->GetWindowButtonVisibility();
}

void BaseWindow::SetWindowButtonPosition(std::optional<gfx::Point> position) {
  window_->SetWindowButtonPosition(std::move(position));
}

std::optional<gfx::Point> BaseWindow::GetWindowButtonPosition() const {
  return window_->GetWindowButtonPosition();
}
#endif

#if BUILDFLAG(IS_MAC)
bool BaseWindow::IsHiddenInMissionControl() {
  return window_->IsHiddenInMissionControl();
}

void BaseWindow::SetHiddenInMissionControl(bool hidden) {
  window_->SetHiddenInMissionControl(hidden);
}
#endif

void BaseWindow::SetTouchBar(
    std::vector<gin_helper::PersistentDictionary> items) {
  window_->SetTouchBar(std::move(items));
}

void BaseWindow::RefreshTouchBarItem(const std::string& item_id) {
  window_->RefreshTouchBarItem(item_id);
}

void BaseWindow::SetEscapeTouchBarItem(gin_helper::PersistentDictionary item) {
  window_->SetEscapeTouchBarItem(std::move(item));
}

void BaseWindow::SelectPreviousTab() {
  window_->SelectPreviousTab();
}

void BaseWindow::SelectNextTab() {
  window_->SelectNextTab();
}

void BaseWindow::ShowAllTabs() {
  window_->ShowAllTabs();
}

void BaseWindow::MergeAllWindows() {
  window_->MergeAllWindows();
}

void BaseWindow::MoveTabToNewWindow() {
  window_->MoveTabToNewWindow();
}

void BaseWindow::ToggleTabBar() {
  window_->ToggleTabBar();
}

void BaseWindow::AddTabbedWindow(NativeWindow* const window,
                                 gin::Arguments* const args) {
  if (!window_->AddTabbedWindow(window))
    args->ThrowTypeError(
        "AddTabbedWindow cannot be called by a window on itself.");
}

v8::Local<v8::Value> BaseWindow::GetTabbingIdentifier() {
  auto tabbing_id = window_->GetTabbingIdentifier();
  if (!tabbing_id.has_value())
    return v8::Undefined(isolate());

  return gin::ConvertToV8(isolate(), tabbing_id.value());
}

void BaseWindow::SetAutoHideMenuBar(bool auto_hide) {
  window_->SetAutoHideMenuBar(auto_hide);
}

bool BaseWindow::IsMenuBarAutoHide() const {
  return window_->IsMenuBarAutoHide();
}

void BaseWindow::SetMenuBarVisibility(bool visible) {
  window_->SetMenuBarVisibility(visible);
}

bool BaseWindow::IsMenuBarVisible() const {
  return window_->IsMenuBarVisible();
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

void BaseWindow::CloseFilePreview() {
  window_->CloseFilePreview();
}

void BaseWindow::SetGTKDarkThemeEnabled(bool use_dark_theme) {
  window_->SetGTKDarkThemeEnabled(use_dark_theme);
}

v8::Local<v8::Value> BaseWindow::GetContentView() const {
  if (content_view_.IsEmpty())
    return v8::Null(isolate());
  else
    return v8::Local<v8::Value>::New(isolate(), content_view_);
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

bool BaseWindow::IsModal() const {
  return window_->is_modal();
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

bool BaseWindow::IsSnapped() const {
  return window_->IsSnapped();
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
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod<&BaseWindow::SetContentView>("setContentView")
      .SetMethod<&BaseWindow::Close>("close")
      .SetMethod<&BaseWindow::Focus>("focus")
      .SetMethod<&BaseWindow::Blur>("blur")
      .SetMethod<&BaseWindow::IsFocused>("isFocused")
      .SetMethod<&BaseWindow::Show>("show")
      .SetMethod<&BaseWindow::ShowInactive>("showInactive")
      .SetMethod<&BaseWindow::Hide>("hide")
      .SetMethod<&BaseWindow::IsVisible>("isVisible")
      .SetMethod<&BaseWindow::IsEnabled>("isEnabled")
      .SetMethod<&BaseWindow::SetEnabled>("setEnabled")
      .SetMethod<&BaseWindow::Maximize>("maximize")
      .SetMethod<&BaseWindow::Unmaximize>("unmaximize")
      .SetMethod<&BaseWindow::IsMaximized>("isMaximized")
      .SetMethod<&BaseWindow::Minimize>("minimize")
      .SetMethod<&BaseWindow::Restore>("restore")
      .SetMethod<&BaseWindow::IsMinimized>("isMinimized")
      .SetMethod<&BaseWindow::SetFullScreen>("setFullScreen")
      .SetMethod<&BaseWindow::IsFullscreen>("isFullScreen")
      .SetProperty<&BaseWindow::IsFullscreen, &BaseWindow::SetFullScreen>(
          "fullScreen")
      .SetMethod<&BaseWindow::SetBounds>("setBounds")
      .SetMethod<&BaseWindow::GetBounds>("getBounds")
      .SetMethod<&BaseWindow::IsNormal>("isNormal")
      .SetMethod<&BaseWindow::GetNormalBounds>("getNormalBounds")
      .SetMethod<&BaseWindow::SetSize>("setSize")
      .SetMethod<&BaseWindow::GetSize>("getSize")
      .SetMethod<&BaseWindow::SetContentBounds>("setContentBounds")
      .SetMethod<&BaseWindow::GetContentBounds>("getContentBounds")
      .SetMethod<&BaseWindow::SetContentSize>("setContentSize")
      .SetMethod<&BaseWindow::GetContentSize>("getContentSize")
      .SetMethod<&BaseWindow::SetMinimumSize>("setMinimumSize")
      .SetMethod<&BaseWindow::GetMinimumSize>("getMinimumSize")
      .SetMethod<&BaseWindow::SetMaximumSize>("setMaximumSize")
      .SetMethod<&BaseWindow::GetMaximumSize>("getMaximumSize")
      .SetMethod<&BaseWindow::SetSheetOffset>("setSheetOffset")
      .SetMethod<&BaseWindow::MoveAbove>("moveAbove")
      .SetMethod<&BaseWindow::MoveTop>("moveTop")
      .SetMethod<&BaseWindow::SetResizable>("setResizable")
      .SetMethod<&BaseWindow::IsResizable>("isResizable")
      .SetProperty<&BaseWindow::IsResizable, &BaseWindow::SetResizable>(
          "resizable")
      .SetMethod<&BaseWindow::SetMovable>("setMovable")
      .SetMethod<&BaseWindow::IsMovable>("isMovable")
      .SetProperty<&BaseWindow::IsMovable, &BaseWindow::SetMovable>("movable")
      .SetMethod<&BaseWindow::SetMinimizable>("setMinimizable")
      .SetMethod<&BaseWindow::IsMinimizable>("isMinimizable")
      .SetProperty<&BaseWindow::IsMinimizable, &BaseWindow::SetMinimizable>(
          "minimizable")
      .SetMethod<&BaseWindow::SetMaximizable>("setMaximizable")
      .SetMethod<&BaseWindow::IsMaximizable>("isMaximizable")
      .SetProperty<&BaseWindow::IsMaximizable, &BaseWindow::SetMaximizable>(
          "maximizable")
      .SetMethod<&BaseWindow::SetFullScreenable>("setFullScreenable")
      .SetMethod<&BaseWindow::IsFullScreenable>("isFullScreenable")
      .SetProperty<&BaseWindow::IsFullScreenable,
                   &BaseWindow::SetFullScreenable>("fullScreenable")
      .SetMethod<&BaseWindow::SetClosable>("setClosable")
      .SetMethod<&BaseWindow::IsClosable>("isClosable")
      .SetProperty<&BaseWindow::IsClosable, &BaseWindow::SetClosable>(
          "closable")
      .SetMethod<&BaseWindow::SetAlwaysOnTop>("setAlwaysOnTop")
      .SetMethod<&BaseWindow::IsAlwaysOnTop>("isAlwaysOnTop")
      .SetMethod<&BaseWindow::Center>("center")
      .SetMethod<&BaseWindow::SetPosition>("setPosition")
      .SetMethod<&BaseWindow::GetPosition>("getPosition")
      .SetMethod<&BaseWindow::SetTitle>("setTitle")
      .SetMethod<&BaseWindow::GetTitle>("getTitle")
      .SetProperty<&BaseWindow::GetTitle, &BaseWindow::SetTitle>("title")
      .SetProperty<&BaseWindow::GetAccessibleTitle,
                   &BaseWindow::SetAccessibleTitle>("accessibleTitle")
      .SetMethod<&BaseWindow::FlashFrame>("flashFrame")
      .SetMethod<&BaseWindow::SetSkipTaskbar>("setSkipTaskbar")
      .SetMethod<&BaseWindow::SetSimpleFullScreen>("setSimpleFullScreen")
      .SetMethod<&BaseWindow::IsSimpleFullScreen>("isSimpleFullScreen")
      .SetProperty<&BaseWindow::IsSimpleFullScreen,
                   &BaseWindow::SetSimpleFullScreen>("simpleFullScreen")
      .SetMethod<&BaseWindow::SetKiosk>("setKiosk")
      .SetMethod<&BaseWindow::IsKiosk>("isKiosk")
      .SetProperty<&BaseWindow::IsKiosk, &BaseWindow::SetKiosk>("kiosk")
      .SetMethod<&BaseWindow::IsTabletMode>("isTabletMode")
      .SetMethod<&BaseWindow::SetBackgroundColor>("setBackgroundColor")
      .SetMethod<&BaseWindow::GetBackgroundColor>("getBackgroundColor")
      .SetMethod<&BaseWindow::SetHasShadow>("setHasShadow")
      .SetMethod<&BaseWindow::HasShadow>("hasShadow")
      .SetProperty<&BaseWindow::HasShadow, &BaseWindow::SetHasShadow>("shadow")
      .SetMethod<&BaseWindow::SetOpacity>("setOpacity")
      .SetMethod<&BaseWindow::GetOpacity>("getOpacity")
      .SetMethod<&BaseWindow::SetShape>("setShape")
      .SetMethod<&BaseWindow::SetRepresentedFilename>("setRepresentedFilename")
      .SetMethod<&BaseWindow::GetRepresentedFilename>("getRepresentedFilename")
      .SetProperty<&BaseWindow::GetRepresentedFilename,
                   &BaseWindow::SetRepresentedFilename>("representedFilename")
      .SetMethod<&BaseWindow::SetDocumentEdited>("setDocumentEdited")
      .SetMethod<&BaseWindow::IsDocumentEdited>("isDocumentEdited")
      .SetProperty<&BaseWindow::IsDocumentEdited,
                   &BaseWindow::SetDocumentEdited>("documentEdited")
      .SetMethod<&BaseWindow::SetIgnoreMouseEvents>("setIgnoreMouseEvents")
      .SetMethod<&BaseWindow::SetContentProtection>("setContentProtection")
      .SetMethod<&BaseWindow::IsContentProtected>("isContentProtected")
      .SetMethod<&BaseWindow::SetFocusable>("setFocusable")
      .SetMethod<&BaseWindow::IsFocusable>("isFocusable")
      .SetProperty<&BaseWindow::IsFocusable, &BaseWindow::SetFocusable>(
          "focusable")
      .SetMethod<&BaseWindow::SetMenu>("setMenu")
      .SetMethod<&BaseWindow::RemoveMenu>("removeMenu")
      .SetMethod<&BaseWindow::SetParentWindow>("setParentWindow")
      .SetMethod<&BaseWindow::GetMediaSourceId>("getMediaSourceId")
      .SetMethod<&BaseWindow::GetNativeWindowHandle>("getNativeWindowHandle")
      .SetMethod<&BaseWindow::SetProgressBar>("setProgressBar")
      .SetMethod<&BaseWindow::SetOverlayIcon>("setOverlayIcon")
      .SetMethod<&BaseWindow::SetVisibleOnAllWorkspaces>(
          "setVisibleOnAllWorkspaces")
      .SetMethod<&BaseWindow::IsVisibleOnAllWorkspaces>(
          "isVisibleOnAllWorkspaces")
      .SetProperty<&BaseWindow::IsVisibleOnAllWorkspaces,
                   &BaseWindow::SetVisibleOnAllWorkspaces>(
          "visibleOnAllWorkspaces")
#if BUILDFLAG(IS_MAC)
      .SetMethod<&BaseWindow::InvalidateShadow>("invalidateShadow")
      .SetMethod<&BaseWindow::GetAlwaysOnTopLevel>("_getAlwaysOnTopLevel")
      .SetMethod<&BaseWindow::SetAutoHideCursor>("setAutoHideCursor")
#endif
      .SetMethod<&BaseWindow::SetVibrancy>("setVibrancy")
      .SetMethod<&BaseWindow::SetBackgroundMaterial>("setBackgroundMaterial")

#if BUILDFLAG(IS_MAC)
      .SetMethod<&BaseWindow::IsHiddenInMissionControl>(
          "isHiddenInMissionControl")
      .SetMethod<&BaseWindow::SetHiddenInMissionControl>(
          "setHiddenInMissionControl")
#endif
      .SetMethod<&BaseWindow::SetTouchBar>("_setTouchBarItems")
      .SetMethod<&BaseWindow::RefreshTouchBarItem>("_refreshTouchBarItem")
      .SetMethod<&BaseWindow::SetEscapeTouchBarItem>("_setEscapeTouchBarItem")
#if BUILDFLAG(IS_MAC)
      .SetMethod<&BaseWindow::SelectPreviousTab>("selectPreviousTab")
      .SetMethod<&BaseWindow::SelectNextTab>("selectNextTab")
      .SetMethod<&BaseWindow::ShowAllTabs>("showAllTabs")
      .SetMethod<&BaseWindow::MergeAllWindows>("mergeAllWindows")
      .SetMethod<&BaseWindow::MoveTabToNewWindow>("moveTabToNewWindow")
      .SetMethod<&BaseWindow::ToggleTabBar>("toggleTabBar")
      .SetMethod<&BaseWindow::AddTabbedWindow>("addTabbedWindow")
      .SetProperty<&BaseWindow::GetTabbingIdentifier>("tabbingIdentifier")
      .SetMethod<&BaseWindow::SetWindowButtonVisibility>(
          "setWindowButtonVisibility")
      .SetMethod<&BaseWindow::GetWindowButtonVisibility>(
          "_getWindowButtonVisibility")
      .SetMethod<&BaseWindow::SetWindowButtonPosition>(
          "setWindowButtonPosition")
      .SetMethod<&BaseWindow::GetWindowButtonPosition>(
          "getWindowButtonPosition")
      .SetProperty<&BaseWindow::IsExcludedFromShownWindowsMenu,
                   &BaseWindow::SetExcludedFromShownWindowsMenu>(
          "excludedFromShownWindowsMenu")
#endif
      .SetMethod<&BaseWindow::SetAutoHideMenuBar>("setAutoHideMenuBar")
      .SetMethod<&BaseWindow::IsMenuBarAutoHide>("isMenuBarAutoHide")
      .SetProperty<&BaseWindow::IsMenuBarAutoHide,
                   &BaseWindow::SetAutoHideMenuBar>("autoHideMenuBar")
      .SetMethod<&BaseWindow::SetMenuBarVisibility>("setMenuBarVisibility")
      .SetMethod<&BaseWindow::IsMenuBarVisible>("isMenuBarVisible")
      .SetProperty<&BaseWindow::IsMenuBarVisible,
                   &BaseWindow::SetMenuBarVisibility>("menuBarVisible")
      .SetMethod<&BaseWindow::SetAspectRatio>("setAspectRatio")
      .SetMethod<&BaseWindow::PreviewFile>("previewFile")
      .SetMethod<&BaseWindow::CloseFilePreview>("closeFilePreview")
      .SetMethod<&BaseWindow::GetContentView>("getContentView")
      .SetProperty<&BaseWindow::GetContentView, &BaseWindow::SetContentView>(
          "contentView")
      .SetMethod<&BaseWindow::GetParentWindow>("getParentWindow")
      .SetMethod<&BaseWindow::GetChildWindows>("getChildWindows")
      .SetMethod<&BaseWindow::IsModal>("isModal")
      .SetMethod<&BaseWindow::SetThumbarButtons>("setThumbarButtons")
#if defined(TOOLKIT_VIEWS)
      .SetMethod<&BaseWindow::SetIcon>("setIcon")
#endif
#if BUILDFLAG(IS_WIN)
      .SetMethod<&BaseWindow::IsSnapped>("isSnapped")
      .SetProperty<&BaseWindow::IsSnapped>("snapped")
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
