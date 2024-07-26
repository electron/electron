// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_tray.h"

#include <string>
#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "gin/dictionary.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/browser/api/ui_event.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/guid_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<electron::TrayIcon::IconType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::TrayIcon::IconType* out) {
    using Val = electron::TrayIcon::IconType;
    static constexpr auto Lookup =
        base::MakeFixedFlatMap<std::string_view, Val>({
            {"custom", Val::kCustom},
            {"error", Val::kError},
            {"info", Val::kInfo},
            {"none", Val::kNone},
            {"warning", Val::kWarning},
        });
    return FromV8WithLookup(isolate, val, Lookup, out);
  }
};

}  // namespace gin

namespace electron::api {

gin::WrapperInfo Tray::kWrapperInfo = {gin::kEmbedderNativeGin};

Tray::Tray(v8::Isolate* isolate,
           v8::Local<v8::Value> image,
           std::optional<UUID> guid)
    : tray_icon_(TrayIcon::Create(guid)) {
  SetImage(isolate, image);
  tray_icon_->AddObserver(this);
}

Tray::~Tray() = default;

// static
gin::Handle<Tray> Tray::New(gin_helper::ErrorThrower thrower,
                            v8::Local<v8::Value> image,
                            std::optional<UUID> guid,
                            gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create Tray before app is ready");
    return gin::Handle<Tray>();
  }

#if BUILDFLAG(IS_WIN)
  if (!guid.has_value() && args->Length() > 1) {
    thrower.ThrowError("Invalid GUID format");
    return gin::Handle<Tray>();
  }
#endif

  // Error thrown by us will be dropped when entering V8.
  // Make sure to abort early and propagate the error to JS.
  // Refs https://chromium-review.googlesource.com/c/v8/v8/+/5050065
  v8::TryCatch try_catch(args->isolate());
  auto* tray = new Tray(args->isolate(), image, guid);
  if (try_catch.HasCaught()) {
    delete tray;
    try_catch.ReThrow();
    return gin::Handle<Tray>();
  }

  auto handle = gin::CreateHandle(args->isolate(), tray);
  handle->Pin(args->isolate());
  return handle;
}

void Tray::OnClicked(const gfx::Rect& bounds,
                     const gfx::Point& location,
                     int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("click", CreateEventFromFlags(modifiers), bounds, location);
}

void Tray::OnDoubleClicked(const gfx::Rect& bounds, int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("double-click", CreateEventFromFlags(modifiers), bounds);
}

void Tray::OnRightClicked(const gfx::Rect& bounds, int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("right-click", CreateEventFromFlags(modifiers), bounds);
}

void Tray::OnMiddleClicked(const gfx::Rect& bounds, int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("middle-click", CreateEventFromFlags(modifiers), bounds);
}

void Tray::OnBalloonShow() {
  Emit("balloon-show");
}

void Tray::OnBalloonClicked() {
  Emit("balloon-click");
}

void Tray::OnBalloonClosed() {
  Emit("balloon-closed");
}

void Tray::OnDrop() {
  Emit("drop");
}

void Tray::OnDropFiles(const std::vector<std::string>& files) {
  Emit("drop-files", files);
}

void Tray::OnDropText(const std::string& text) {
  Emit("drop-text", text);
}

void Tray::OnMouseEntered(const gfx::Point& location, int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("mouse-enter", CreateEventFromFlags(modifiers), location);
}

void Tray::OnMouseExited(const gfx::Point& location, int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("mouse-leave", CreateEventFromFlags(modifiers), location);
}

void Tray::OnMouseMoved(const gfx::Point& location, int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("mouse-move", CreateEventFromFlags(modifiers), location);
}

void Tray::OnMouseUp(const gfx::Point& location, int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("mouse-up", CreateEventFromFlags(modifiers), location);
}

void Tray::OnMouseDown(const gfx::Point& location, int modifiers) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("mouse-down", CreateEventFromFlags(modifiers), location);
}

void Tray::OnDragEntered() {
  Emit("drag-enter");
}

void Tray::OnDragExited() {
  Emit("drag-leave");
}

void Tray::OnDragEnded() {
  Emit("drag-end");
}

void Tray::Destroy() {
  Unpin();
  menu_.Reset();
  tray_icon_.reset();
}

bool Tray::IsDestroyed() {
  return !tray_icon_;
}

void Tray::SetImage(v8::Isolate* isolate, v8::Local<v8::Value> image) {
  if (!CheckAlive())
    return;

  NativeImage* native_image = nullptr;
  if (!NativeImage::TryConvertNativeImage(isolate, image, &native_image))
    return;

#if BUILDFLAG(IS_WIN)
  tray_icon_->SetImage(native_image->GetHICON(GetSystemMetrics(SM_CXSMICON)));
#else
  tray_icon_->SetImage(native_image->image());
#endif
}

void Tray::SetPressedImage(v8::Isolate* isolate, v8::Local<v8::Value> image) {
  if (!CheckAlive())
    return;

  NativeImage* native_image = nullptr;
  if (!NativeImage::TryConvertNativeImage(isolate, image, &native_image))
    return;

#if BUILDFLAG(IS_WIN)
  tray_icon_->SetPressedImage(
      native_image->GetHICON(GetSystemMetrics(SM_CXSMICON)));
#else
  tray_icon_->SetPressedImage(native_image->image());
#endif
}

void Tray::SetToolTip(const std::string& tool_tip) {
  if (!CheckAlive())
    return;
  tray_icon_->SetToolTip(tool_tip);
}

void Tray::SetTitle(const std::string& title,
                    const std::optional<gin_helper::Dictionary>& options,
                    gin::Arguments* args) {
  if (!CheckAlive())
    return;
#if BUILDFLAG(IS_MAC)
  TrayIcon::TitleOptions title_options;
  if (options) {
    if (options->Get("fontType", &title_options.font_type)) {
      // Validate the font type if it's passed in
      if (title_options.font_type != "monospaced" &&
          title_options.font_type != "monospacedDigit") {
        args->ThrowTypeError(
            "fontType must be one of 'monospaced' or 'monospacedDigit'");
        return;
      }
    } else if (options->Has("fontType")) {
      args->ThrowTypeError(
          "fontType must be one of 'monospaced' or 'monospacedDigit'");
      return;
    }
  } else if (args->Length() >= 2) {
    args->ThrowTypeError("setTitle options must be an object");
    return;
  }

  tray_icon_->SetTitle(title, title_options);
#endif
}

std::string Tray::GetTitle() {
  if (!CheckAlive())
    return std::string();
#if BUILDFLAG(IS_MAC)
  return tray_icon_->GetTitle();
#else
  return "";
#endif
}

void Tray::SetIgnoreDoubleClickEvents(bool ignore) {
  if (!CheckAlive())
    return;
#if BUILDFLAG(IS_MAC)
  tray_icon_->SetIgnoreDoubleClickEvents(ignore);
#endif
}

bool Tray::GetIgnoreDoubleClickEvents() {
  if (!CheckAlive())
    return false;
#if BUILDFLAG(IS_MAC)
  return tray_icon_->GetIgnoreDoubleClickEvents();
#else
  return false;
#endif
}

void Tray::DisplayBalloon(gin_helper::ErrorThrower thrower,
                          const gin_helper::Dictionary& options) {
  if (!CheckAlive())
    return;
  TrayIcon::BalloonOptions balloon_options;

  if (!options.Get("title", &balloon_options.title) ||
      !options.Get("content", &balloon_options.content)) {
    thrower.ThrowError("'title' and 'content' must be defined");
    return;
  }

  v8::Local<v8::Value> icon_value;
  NativeImage* icon = nullptr;
  if (options.Get("icon", &icon_value) &&
      !NativeImage::TryConvertNativeImage(thrower.isolate(), icon_value,
                                          &icon)) {
    return;
  }

  options.Get("iconType", &balloon_options.icon_type);
  options.Get("largeIcon", &balloon_options.large_icon);
  options.Get("noSound", &balloon_options.no_sound);
  options.Get("respectQuietTime", &balloon_options.respect_quiet_time);

  if (icon) {
#if BUILDFLAG(IS_WIN)
    balloon_options.icon = icon->GetHICON(
        GetSystemMetrics(balloon_options.large_icon ? SM_CXICON : SM_CXSMICON));
#else
    balloon_options.icon = icon->image();
#endif
  }

  tray_icon_->DisplayBalloon(balloon_options);
}

void Tray::RemoveBalloon() {
  if (!CheckAlive())
    return;
  tray_icon_->RemoveBalloon();
}

void Tray::Focus() {
  if (!CheckAlive())
    return;
  tray_icon_->Focus();
}

void Tray::PopUpContextMenu(gin::Arguments* args) {
  if (!CheckAlive())
    return;
  gin::Handle<Menu> menu;
  gfx::Point pos;

  v8::Local<v8::Value> first_arg;
  if (args->GetNext(&first_arg)) {
    if (!gin::ConvertFromV8(args->isolate(), first_arg, &menu)) {
      if (!gin::ConvertFromV8(args->isolate(), first_arg, &pos)) {
        args->ThrowError();
        return;
      }
    } else if (args->Length() >= 2) {
      if (!args->GetNext(&pos)) {
        args->ThrowError();
        return;
      }
    }
  }

  tray_icon_->PopUpContextMenu(
      pos, menu.IsEmpty() ? nullptr : menu->model()->GetWeakPtr());
}

void Tray::CloseContextMenu() {
  if (!CheckAlive())
    return;
  tray_icon_->CloseContextMenu();
}

void Tray::SetContextMenu(gin_helper::ErrorThrower thrower,
                          v8::Local<v8::Value> arg) {
  if (!CheckAlive())
    return;
  gin::Handle<Menu> menu;
  if (arg->IsNull()) {
    menu_.Reset();
    tray_icon_->SetContextMenu(nullptr);
  } else if (gin::ConvertFromV8(thrower.isolate(), arg, &menu)) {
    menu_.Reset(thrower.isolate(), menu.ToV8());
    tray_icon_->SetContextMenu(menu->model());
  } else {
    thrower.ThrowTypeError("Must pass Menu or null");
  }
}

gfx::Rect Tray::GetBounds() {
  if (!CheckAlive())
    return gfx::Rect();
  return tray_icon_->GetBounds();
}

bool Tray::CheckAlive() {
  if (!tray_icon_) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    gin_helper::ErrorThrower(isolate).ThrowError("Tray is destroyed");
    return false;
  }
  return true;
}

// static
void Tray::FillObjectTemplate(v8::Isolate* isolate,
                              v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, GetClassName(), templ)
      .SetMethod("destroy", &Tray::Destroy)
      .SetMethod("isDestroyed", &Tray::IsDestroyed)
      .SetMethod("setImage", &Tray::SetImage)
      .SetMethod("setPressedImage", &Tray::SetPressedImage)
      .SetMethod("setToolTip", &Tray::SetToolTip)
      .SetMethod("setTitle", &Tray::SetTitle)
      .SetMethod("getTitle", &Tray::GetTitle)
      .SetMethod("setIgnoreDoubleClickEvents",
                 &Tray::SetIgnoreDoubleClickEvents)
      .SetMethod("getIgnoreDoubleClickEvents",
                 &Tray::GetIgnoreDoubleClickEvents)
      .SetMethod("displayBalloon", &Tray::DisplayBalloon)
      .SetMethod("removeBalloon", &Tray::RemoveBalloon)
      .SetMethod("focus", &Tray::Focus)
      .SetMethod("popUpContextMenu", &Tray::PopUpContextMenu)
      .SetMethod("closeContextMenu", &Tray::CloseContextMenu)
      .SetMethod("setContextMenu", &Tray::SetContextMenu)
      .SetMethod("getBounds", &Tray::GetBounds)
      .Build();
}

const char* Tray::GetTypeName() {
  return GetClassName();
}

}  // namespace electron::api

namespace {

using electron::api::Tray;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  gin::Dictionary dict(isolate, exports);
  dict.Set("Tray", Tray::GetConstructor(context));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_tray, Initialize)
