// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_window.h"

#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/browser.h"
#include "atom/browser/native_window.h"
#include "atom/common/options_switches.h"
#include "atom/common/event_types.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/geometry/rect.h"
#include "v8/include/v8.h"

#if defined(OS_WIN)
#include "atom/browser/native_window_views.h"
#include "atom/browser/ui/win/taskbar_host.h"
#endif

#include "atom/common/node_includes.h"

#if defined(OS_WIN)
namespace mate {

template<>
struct Converter<atom::TaskbarHost::ThumbarButton> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     atom::TaskbarHost::ThumbarButton* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    dict.Get("click", &(out->clicked_callback));
    dict.Get("tooltip", &(out->tooltip));
    dict.Get("flags", &out->flags);
    return dict.Get("icon", &(out->icon));
  }
};

}  // namespace mate
#endif

namespace atom {

namespace api {

namespace {

void OnCapturePageDone(
    v8::Isolate* isolate,
    const base::Callback<void(const gfx::Image&)>& callback,
    const SkBitmap& bitmap) {
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  callback.Run(gfx::Image::CreateFrom1xBitmap(bitmap));
}

}  // namespace


Window::Window(v8::Isolate* isolate, const mate::Dictionary& options) {
  // Creates the WebContents used by BrowserWindow.
  mate::Dictionary web_contents_options(isolate, v8::Object::New(isolate));
  auto web_contents = WebContents::Create(isolate, web_contents_options);
  web_contents_.Reset(isolate, web_contents.ToV8());
  api_web_contents_ = web_contents.get();

  // Creates BrowserWindow.
  window_.reset(NativeWindow::Create(web_contents->managed_web_contents(),
                                     options));
  web_contents->SetOwnerWindow(window_.get());
  window_->InitFromOptions(options);
  window_->AddObserver(this);
}

Window::~Window() {
  if (window_)
    Destroy();
}

void Window::OnPageTitleUpdated(bool* prevent_default,
                                const std::string& title) {
  *prevent_default = Emit("page-title-updated", title);
}

void Window::WillCloseWindow(bool* prevent_default) {
  *prevent_default = Emit("close");
}

void Window::OnFrameRendered(scoped_ptr<uint8[]> rgb, const int size) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());

  v8::Local<v8::ArrayBuffer> data = v8::ArrayBuffer::New(isolate(), rgb.get(), size);
  v8::Local<v8::Uint8ClampedArray> uint_data = v8::Uint8ClampedArray::New(data, 0, size);

  Emit("frame-rendered", uint_data, size);
}

void Window::OnWindowClosed() {
  if (api_web_contents_) {
    api_web_contents_->DestroyWebContents();
    api_web_contents_ = nullptr;
    web_contents_.Reset();
  }

  RemoveFromWeakMap();
  window_->RemoveObserver(this);

  Emit("closed");
}

void Window::OnWindowBlur() {
  Emit("blur");
}

void Window::OnWindowFocus() {
  Emit("focus");
}

void Window::OnWindowMaximize() {
  Emit("maximize");
}

void Window::OnWindowUnmaximize() {
  Emit("unmaximize");
}

void Window::OnWindowMinimize() {
  Emit("minimize");
}

void Window::OnWindowRestore() {
  Emit("restore");
}

void Window::OnWindowResize() {
  Emit("resize");
}

void Window::OnWindowMove() {
  Emit("move");
}

void Window::OnWindowMoved() {
  Emit("moved");
}

void Window::OnWindowEnterFullScreen() {
  Emit("enter-full-screen");
}

void Window::OnWindowLeaveFullScreen() {
  Emit("leave-full-screen");
}

void Window::OnWindowEnterHtmlFullScreen() {
  Emit("enter-html-full-screen");
}

void Window::OnWindowLeaveHtmlFullScreen() {
  Emit("leave-html-full-screen");
}

void Window::OnRendererUnresponsive() {
  Emit("unresponsive");
}

void Window::OnRendererResponsive() {
  Emit("responsive");
}

void Window::OnDevToolsFocus() {
  Emit("devtools-focused");
}

void Window::OnDevToolsOpened() {
  Emit("devtools-opened");

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto handle = WebContents::CreateFrom(
      isolate(), api_web_contents_->GetDevToolsWebContents());
  devtools_web_contents_.Reset(isolate(), handle.ToV8());
}

void Window::OnDevToolsClosed() {
  Emit("devtools-closed");

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  devtools_web_contents_.Reset();
}

void Window::OnExecuteWindowsCommand(const std::string& command_name) {
  Emit("app-command", command_name);
}

// static
mate::Wrappable* Window::New(v8::Isolate* isolate,
                             const mate::Dictionary& options) {
  if (!Browser::Get()->is_ready()) {
    node::ThrowError(isolate,
                     "Cannot create BrowserWindow before app is ready");
    return nullptr;
  }
  return new Window(isolate, options);
}

bool Window::IsDestroyed() const {
  return !window_ || window_->IsClosed();
}

void Window::Destroy() {
  window_->CloseContents(nullptr);
}

void Window::Close() {
  window_->Close();
}

bool Window::IsClosed() {
  return window_->IsClosed();
}

void Window::Focus() {
  window_->Focus(true);
}

bool Window::IsFocused() {
  return window_->IsFocused();
}

void Window::Show() {
  window_->Show();
}

void Window::ShowInactive() {
  window_->ShowInactive();
}

void Window::Hide() {
  window_->Hide();
}

bool Window::IsVisible() {
  return window_->IsVisible();
}

void Window::Maximize() {
  window_->Maximize();
}

void Window::Unmaximize() {
  window_->Unmaximize();
}

bool Window::IsMaximized() {
  return window_->IsMaximized();
}

void Window::Minimize() {
  window_->Minimize();
}

void Window::Restore() {
  window_->Restore();
}

bool Window::IsMinimized() {
  return window_->IsMinimized();
}

void Window::SetFullScreen(bool fullscreen) {
  window_->SetFullScreen(fullscreen);
}

bool Window::IsFullscreen() {
  return window_->IsFullscreen();
}

void Window::SetBounds(const gfx::Rect& bounds) {
  window_->SetBounds(bounds);
}

gfx::Rect Window::GetBounds() {
  return window_->GetBounds();
}

void Window::SetSize(int width, int height) {
  window_->SetSize(gfx::Size(width, height));
}

std::vector<int> Window::GetSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetContentSize(int width, int height) {
  window_->SetContentSize(gfx::Size(width, height));
}

std::vector<int> Window::GetContentSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetContentSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetMinimumSize(int width, int height) {
  window_->SetMinimumSize(gfx::Size(width, height));
}

std::vector<int> Window::GetMinimumSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetMinimumSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetMaximumSize(int width, int height) {
  window_->SetMaximumSize(gfx::Size(width, height));
}

std::vector<int> Window::GetMaximumSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetMaximumSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetResizable(bool resizable) {
  window_->SetResizable(resizable);
}

bool Window::IsResizable() {
  return window_->IsResizable();
}

void Window::SetAlwaysOnTop(bool top) {
  window_->SetAlwaysOnTop(top);
}

bool Window::IsAlwaysOnTop() {
  return window_->IsAlwaysOnTop();
}

void Window::Center() {
  window_->Center();
}

void Window::SetPosition(int x, int y) {
  window_->SetPosition(gfx::Point(x, y));
}

std::vector<int> Window::GetPosition() {
  std::vector<int> result(2);
  gfx::Point pos = window_->GetPosition();
  result[0] = pos.x();
  result[1] = pos.y();
  return result;
}

void Window::SetTitle(const std::string& title) {
  window_->SetTitle(title);
}

std::string Window::GetTitle() {
  return window_->GetTitle();
}

void Window::FlashFrame(bool flash) {
  window_->FlashFrame(flash);
}

void Window::SetSkipTaskbar(bool skip) {
  window_->SetSkipTaskbar(skip);
}

void Window::SetKiosk(bool kiosk) {
  window_->SetKiosk(kiosk);
}

bool Window::IsKiosk() {
  return window_->IsKiosk();
}

void Window::FocusOnWebView() {
  window_->FocusOnWebView();
}

void Window::BlurWebView() {
  window_->BlurWebView();
}

bool Window::IsWebViewFocused() {
  return window_->IsWebViewFocused();
}

void Window::SetRepresentedFilename(const std::string& filename) {
  window_->SetRepresentedFilename(filename);
}

std::string Window::GetRepresentedFilename() {
  return window_->GetRepresentedFilename();
}

void Window::SetDocumentEdited(bool edited) {
  window_->SetDocumentEdited(edited);
}

bool Window::IsDocumentEdited() {
  return window_->IsDocumentEdited();
}

void Window::CapturePage(mate::Arguments* args) {
  gfx::Rect rect;
  base::Callback<void(const gfx::Image&)> callback;

  if (!(args->Length() == 1 && args->GetNext(&callback)) &&
      !(args->Length() == 2 && args->GetNext(&rect)
                            && args->GetNext(&callback))) {
    args->ThrowError();
    return;
  }

  window_->CapturePage(
      rect, base::Bind(&OnCapturePageDone, args->isolate(), callback));
}

void Window::SetOffscreenRender(bool isOffscreen) {
  window_->SetOffscreenRender(isOffscreen);
}

void Window::SetProgressBar(double progress) {
  window_->SetProgressBar(progress);
}

void Window::SetOverlayIcon(const gfx::Image& overlay,
                            const std::string& description) {
  window_->SetOverlayIcon(overlay, description);
}

bool Window::SetThumbarButtons(mate::Arguments* args) {
#if defined(OS_WIN)
  std::vector<TaskbarHost::ThumbarButton> buttons;
  if (!args->GetNext(&buttons)) {
    args->ThrowError();
    return false;
  }
  auto window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbarButtons(
      window->GetAcceleratedWidget(), buttons);
#else
  return false;
#endif
}

void Window::SetMenu(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  mate::Handle<Menu> menu;
  if (value->IsObject() &&
      mate::V8ToString(value->ToObject()->GetConstructorName()) == "Menu" &&
      mate::ConvertFromV8(isolate, value, &menu)) {
    menu_.Reset(isolate, menu.ToV8());
    window_->SetMenu(menu->model());
  } else if (value->IsNull()) {
    menu_.Reset();
    window_->SetMenu(nullptr);
  } else {
    isolate->ThrowException(v8::Exception::TypeError(
        mate::StringToV8(isolate, "Invalid Menu")));
  }
}

void Window::SetAutoHideMenuBar(bool auto_hide) {
  window_->SetAutoHideMenuBar(auto_hide);
}

bool Window::IsMenuBarAutoHide() {
  return window_->IsMenuBarAutoHide();
}

void Window::SetMenuBarVisibility(bool visible) {
  window_->SetMenuBarVisibility(visible);
}

bool Window::IsMenuBarVisible() {
  return window_->IsMenuBarVisible();
}

#if defined(OS_MACOSX)
void Window::ShowDefinitionForSelection() {
  window_->ShowDefinitionForSelection();
}
#endif

void Window::SetAspectRatio(double aspect_ratio, mate::Arguments* args) {
  gfx::Size extra_size;
  args->GetNext(&extra_size);
  window_->SetAspectRatio(aspect_ratio, extra_size);
}

void Window::SetVisibleOnAllWorkspaces(bool visible) {
  return window_->SetVisibleOnAllWorkspaces(visible);
}

bool Window::IsVisibleOnAllWorkspaces() {
  return window_->IsVisibleOnAllWorkspaces();
}

void Window::SendKeyboardEvent(v8::Isolate* isolate, const mate::Dictionary& data){
  auto type = blink::WebInputEvent::Type::Char;
  int modifiers = 0;
  int keycode = 0;
  std::string type_str = "";
  std::vector<std::string> modifier_array;

  data.Get(switches::kMouseEventType, &type_str);
  data.Get(switches::kModifiers, &modifier_array);
  data.Get(switches::kKeyCode, &keycode);

  if(type_str.compare(event_types::kKeyDown) == 0){
    type = blink::WebInputEvent::Type::KeyDown;
  }else if(type_str.compare(event_types::kKeyUp) == 0){
    type = blink::WebInputEvent::Type::KeyUp;
  }else if(type_str.compare(event_types::kChar) == 0){
    type = blink::WebInputEvent::Type::Char;
  }

  for(std::vector<std::string>::iterator mod = modifier_array.begin(); mod != modifier_array.end(); ++mod) {
    if(mod->compare(event_types::kModifierIsKeyPad) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::IsKeyPad;
    }else if(mod->compare(event_types::kModifierIsAutoRepeat) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::IsAutoRepeat;
    }else if(mod->compare(event_types::kModifierIsLeft) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::IsLeft;
    }else if(mod->compare(event_types::kModifierIsRight) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::IsRight;
    }else if(mod->compare(event_types::kModifierShiftKey) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::ShiftKey;
    }else if(mod->compare(event_types::kModifierControlKey) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::ControlKey;
    }else if(mod->compare(event_types::kModifierAltKey) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::AltKey;
    }else if(mod->compare(event_types::kModifierMetaKey) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::MetaKey;
    }else if(mod->compare(event_types::kModifierCapsLockOn) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::CapsLockOn;
    }else if(mod->compare(event_types::kModifierNumLockOn) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::NumLockOn;
    }
  }

  window_->SendKeyboardEvent(type, modifiers, keycode);
}

void Window::SendMouseEvent(v8::Isolate* isolate, const mate::Dictionary& data){
  int x, y, movementX, movementY, clickCount;
  std::string type_str = "";
  std::string button_str = "";
  std::vector<std::string> modifier_array;

  blink::WebInputEvent::Type type = blink::WebInputEvent::Type::MouseMove;
  blink::WebMouseEvent::Button button = blink::WebMouseEvent::Button::ButtonNone;
  int modifiers = 0;

  data.Get(switches::kMouseEventType, &type_str);
  data.Get(switches::kMouseEventButton, &button_str);
  data.Get(switches::kModifiers, &modifier_array);

  if(type_str.compare(event_types::kMouseDown) == 0){
    type = blink::WebInputEvent::Type::MouseDown;
  }else if(type_str.compare(event_types::kMouseUp) == 0){
    type = blink::WebInputEvent::Type::MouseUp;
  }else if(type_str.compare(event_types::kMouseMove) == 0){
    type = blink::WebInputEvent::Type::MouseMove;
  }else if(type_str.compare(event_types::kMouseEnter) == 0){
    type = blink::WebInputEvent::Type::MouseEnter;
  }else if(type_str.compare(event_types::kMouseLeave) == 0){
    type = blink::WebInputEvent::Type::MouseLeave;
  }else if(type_str.compare(event_types::kContextMenu) == 0){
    type = blink::WebInputEvent::Type::ContextMenu;
  }else if(type_str.compare(event_types::kMouseWheel) == 0){
    type = blink::WebInputEvent::Type::MouseWheel;
  }

  if(button_str.compare(event_types::kMouseLeftButton) == 0){
    modifiers = modifiers & blink::WebInputEvent::Modifiers::LeftButtonDown;
    button = blink::WebMouseEvent::Button::ButtonLeft;
  }else if(button_str.compare(event_types::kMouseRightButton) == 0){
    modifiers = modifiers & blink::WebInputEvent::Modifiers::RightButtonDown;
    button = blink::WebMouseEvent::Button::ButtonRight;
  }else if(button_str.compare(event_types::kMouseMiddleButton) == 0){
    modifiers = modifiers & blink::WebInputEvent::Modifiers::MiddleButtonDown;
    button = blink::WebMouseEvent::Button::ButtonMiddle;
  }

  for(std::vector<std::string>::iterator mod = modifier_array.begin(); mod != modifier_array.end(); ++mod) {
    if(mod->compare(event_types::kModifierLeftButtonDown) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::LeftButtonDown;
    }else if(mod->compare(event_types::kModifierMiddleButtonDown) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::MiddleButtonDown;
    }else if(mod->compare(event_types::kModifierRightButtonDown) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::RightButtonDown;
    }else if(mod->compare(event_types::kModifierShiftKey) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::ShiftKey;
    }else if(mod->compare(event_types::kModifierControlKey) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::ControlKey;
    }else if(mod->compare(event_types::kModifierAltKey) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::AltKey;
    }else if(mod->compare(event_types::kModifierMetaKey) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::MetaKey;
    }else if(mod->compare(event_types::kModifierCapsLockOn) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::CapsLockOn;
    }else if(mod->compare(event_types::kModifierNumLockOn) == 0){
      modifiers = modifiers & blink::WebInputEvent::Modifiers::NumLockOn;
    }
  }

  if(type == blink::WebInputEvent::Type::MouseWheel){
    bool precise = true;

    x = 0;
    y = 0;
    data.Get(switches::kX, &x);
    data.Get(switches::kY, &y);
    data.Get(switches::kMouseWheelPrecise, &precise);

    window_->SendMouseWheelEvent(modifiers, x, y, precise);
  }else{
    if (data.Get(switches::kX, &x) && data.Get(switches::kY, &y)) {
      if(!data.Get(switches::kMovementX, &movementX)){
        movementX = 0;
      }

      if(!data.Get(switches::kMovementY, &movementY)){
        movementY = 0;
      }

      if(!data.Get(switches::kClickCount, &clickCount)){
        clickCount = 0;
      }

      window_->SendMouseEvent(type, modifiers, button, x, y, movementX, movementY, clickCount);
    }
  }
}

int32_t Window::ID() const {
  return weak_map_id();
}

v8::Local<v8::Value> Window::WebContents(v8::Isolate* isolate) {
  if (web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, web_contents_);
}

v8::Local<v8::Value> Window::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

// static
void Window::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("destroy", &Window::Destroy, true)
      .SetMethod("close", &Window::Close)
      .SetMethod("isClosed", &Window::IsClosed)
      .SetMethod("focus", &Window::Focus)
      .SetMethod("isFocused", &Window::IsFocused)
      .SetMethod("show", &Window::Show)
      .SetMethod("showInactive", &Window::ShowInactive)
      .SetMethod("hide", &Window::Hide)
      .SetMethod("isVisible", &Window::IsVisible)
      .SetMethod("maximize", &Window::Maximize)
      .SetMethod("unmaximize", &Window::Unmaximize)
      .SetMethod("isMaximized", &Window::IsMaximized)
      .SetMethod("minimize", &Window::Minimize)
      .SetMethod("restore", &Window::Restore)
      .SetMethod("isMinimized", &Window::IsMinimized)
      .SetMethod("setFullScreen", &Window::SetFullScreen)
      .SetMethod("isFullScreen", &Window::IsFullscreen)
      .SetMethod("setAspectRatio", &Window::SetAspectRatio)
      .SetMethod("getBounds", &Window::GetBounds)
      .SetMethod("setBounds", &Window::SetBounds)
      .SetMethod("getSize", &Window::GetSize)
      .SetMethod("setSize", &Window::SetSize)
      .SetMethod("getContentSize", &Window::GetContentSize)
      .SetMethod("setContentSize", &Window::SetContentSize)
      .SetMethod("setMinimumSize", &Window::SetMinimumSize)
      .SetMethod("getMinimumSize", &Window::GetMinimumSize)
      .SetMethod("setMaximumSize", &Window::SetMaximumSize)
      .SetMethod("getMaximumSize", &Window::GetMaximumSize)
      .SetMethod("setResizable", &Window::SetResizable)
      .SetMethod("isResizable", &Window::IsResizable)
      .SetMethod("setAlwaysOnTop", &Window::SetAlwaysOnTop)
      .SetMethod("isAlwaysOnTop", &Window::IsAlwaysOnTop)
      .SetMethod("center", &Window::Center)
      .SetMethod("setPosition", &Window::SetPosition)
      .SetMethod("getPosition", &Window::GetPosition)
      .SetMethod("setTitle", &Window::SetTitle)
      .SetMethod("getTitle", &Window::GetTitle)
      .SetMethod("flashFrame", &Window::FlashFrame)
      .SetMethod("setSkipTaskbar", &Window::SetSkipTaskbar)
      .SetMethod("setKiosk", &Window::SetKiosk)
      .SetMethod("isKiosk", &Window::IsKiosk)
      .SetMethod("setRepresentedFilename", &Window::SetRepresentedFilename)
      .SetMethod("getRepresentedFilename", &Window::GetRepresentedFilename)
      .SetMethod("setDocumentEdited", &Window::SetDocumentEdited)
      .SetMethod("isDocumentEdited", &Window::IsDocumentEdited)
      .SetMethod("focusOnWebView", &Window::FocusOnWebView)
      .SetMethod("blurWebView", &Window::BlurWebView)
      .SetMethod("isWebViewFocused", &Window::IsWebViewFocused)
      .SetMethod("capturePage", &Window::CapturePage)
      .SetMethod("setProgressBar", &Window::SetProgressBar)
      .SetMethod("setOverlayIcon", &Window::SetOverlayIcon)
      .SetMethod("setThumbarButtons", &Window::SetThumbarButtons)
      .SetMethod("setMenu", &Window::SetMenu)
      .SetMethod("setAutoHideMenuBar", &Window::SetAutoHideMenuBar)
      .SetMethod("isMenuBarAutoHide", &Window::IsMenuBarAutoHide)
      .SetMethod("setMenuBarVisibility", &Window::SetMenuBarVisibility)
      .SetMethod("isMenuBarVisible", &Window::IsMenuBarVisible)
      .SetMethod("setVisibleOnAllWorkspaces",
                 &Window::SetVisibleOnAllWorkspaces)
      .SetMethod("isVisibleOnAllWorkspaces",
                 &Window::IsVisibleOnAllWorkspaces)
      .SetMethod("sendMouseEvent", &Window::SendMouseEvent)
      .SetMethod("sendKeyboardEvent", &Window::SendKeyboardEvent)
      .SetMethod("setOffscreenRender", &Window::SetOffscreenRender)
#if defined(OS_MACOSX)
      .SetMethod("showDefinitionForSelection",
                 &Window::ShowDefinitionForSelection)
#endif
      .SetProperty("id", &Window::ID, true)
      .SetProperty("webContents", &Window::WebContents, true)
      .SetProperty("devToolsWebContents", &Window::DevToolsWebContents, true);
}

}  // namespace api

}  // namespace atom


namespace {

using atom::api::Window;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Function> constructor = mate::CreateConstructor<Window>(
      isolate, "BrowserWindow", base::Bind(&Window::New));
  mate::Dictionary browser_window(isolate, constructor);
  browser_window.SetMethod("fromId",
                           &mate::TrackableObject<Window>::FromWeakMapID);
  browser_window.SetMethod("getAllWindows",
                           &mate::TrackableObject<Window>::GetAll);

  mate::Dictionary dict(isolate, exports);
  dict.Set("BrowserWindow", browser_window);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_window, Initialize)
