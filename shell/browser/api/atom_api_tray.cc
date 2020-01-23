// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_tray.h"

#include <string>

#include "base/threading/thread_task_runner_handle.h"
#include "shell/browser/api/atom_api_menu.h"
#include "shell/browser/browser.h"
#include "shell/common/api/atom_api_native_image.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/image/image.h"

namespace gin {

template <>
struct Converter<electron::TrayIcon::IconType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::TrayIcon::IconType* out) {
    using IconType = electron::TrayIcon::IconType;
    std::string mode;
    if (ConvertFromV8(isolate, val, &mode)) {
      if (mode == "none") {
        *out = IconType::None;
        return true;
      } else if (mode == "info") {
        *out = IconType::Info;
        return true;
      } else if (mode == "warning") {
        *out = IconType::Warning;
        return true;
      } else if (mode == "error") {
        *out = IconType::Error;
        return true;
      } else if (mode == "custom") {
        *out = IconType::Custom;
        return true;
      }
    }
    return false;
  }
};

}  // namespace gin

namespace electron {

namespace api {

Tray::Tray(gin::Handle<NativeImage> image, gin_helper::Arguments* args)
    : tray_icon_(TrayIcon::Create()) {
  SetImage(args->isolate(), image);
  tray_icon_->AddObserver(this);

  InitWithArgs(args);
}

Tray::~Tray() = default;

// static
gin_helper::WrappableBase* Tray::New(gin_helper::ErrorThrower thrower,
                                     gin::Handle<NativeImage> image,
                                     gin_helper::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create Tray before app is ready");
    return nullptr;
  }
  return new Tray(image, args);
}

void Tray::OnClicked(const gfx::Rect& bounds,
                     const gfx::Point& location,
                     int modifiers) {
  EmitWithFlags("click", modifiers, bounds, location);
}

void Tray::OnDoubleClicked(const gfx::Rect& bounds, int modifiers) {
  EmitWithFlags("double-click", modifiers, bounds);
}

void Tray::OnRightClicked(const gfx::Rect& bounds, int modifiers) {
  EmitWithFlags("right-click", modifiers, bounds);
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
  EmitWithFlags("mouse-enter", modifiers, location);
}

void Tray::OnMouseExited(const gfx::Point& location, int modifiers) {
  EmitWithFlags("mouse-leave", modifiers, location);
}

void Tray::OnMouseMoved(const gfx::Point& location, int modifiers) {
  EmitWithFlags("mouse-move", modifiers, location);
}

void Tray::OnMouseUp(const gfx::Point& location, int modifiers) {
  EmitWithFlags("mouse-up", modifiers, location);
}

void Tray::OnMouseDown(const gfx::Point& location, int modifiers) {
  EmitWithFlags("mouse-down", modifiers, location);
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

void Tray::SetImage(v8::Isolate* isolate, gin::Handle<NativeImage> image) {
#if defined(OS_WIN)
  tray_icon_->SetImage(image->GetHICON(GetSystemMetrics(SM_CXSMICON)));
#else
  tray_icon_->SetImage(image->image());
#endif
}

void Tray::SetPressedImage(v8::Isolate* isolate,
                           gin::Handle<NativeImage> image) {
#if defined(OS_WIN)
  tray_icon_->SetPressedImage(image->GetHICON(GetSystemMetrics(SM_CXSMICON)));
#else
  tray_icon_->SetPressedImage(image->image());
#endif
}

void Tray::SetToolTip(const std::string& tool_tip) {
  tray_icon_->SetToolTip(tool_tip);
}

void Tray::SetTitle(const std::string& title) {
#if defined(OS_MACOSX)
  tray_icon_->SetTitle(title);
#endif
}

std::string Tray::GetTitle() {
#if defined(OS_MACOSX)
  return tray_icon_->GetTitle();
#else
  return "";
#endif
}

void Tray::SetIgnoreDoubleClickEvents(bool ignore) {
#if defined(OS_MACOSX)
  tray_icon_->SetIgnoreDoubleClickEvents(ignore);
#endif
}

bool Tray::GetIgnoreDoubleClickEvents() {
#if defined(OS_MACOSX)
  return tray_icon_->GetIgnoreDoubleClickEvents();
#else
  return false;
#endif
}

void Tray::DisplayBalloon(gin_helper::ErrorThrower thrower,
                          const gin_helper::Dictionary& options) {
  TrayIcon::BalloonOptions balloon_options;

  if (!options.Get("title", &balloon_options.title) ||
      !options.Get("content", &balloon_options.content)) {
    thrower.ThrowError("'title' and 'content' must be defined");
    return;
  }

  gin::Handle<NativeImage> icon;
  options.Get("icon", &icon);
  options.Get("iconType", &balloon_options.icon_type);
  options.Get("largeIcon", &balloon_options.large_icon);
  options.Get("noSound", &balloon_options.no_sound);
  options.Get("respectQuietTime", &balloon_options.respect_quiet_time);

  if (!icon.IsEmpty()) {
#if defined(OS_WIN)
    balloon_options.icon = icon->GetHICON(
        GetSystemMetrics(balloon_options.large_icon ? SM_CXICON : SM_CXSMICON));
#else
    balloon_options.icon = icon->image();
#endif
  }

  tray_icon_->DisplayBalloon(balloon_options);
}

void Tray::RemoveBalloon() {
  tray_icon_->RemoveBalloon();
}

void Tray::Focus() {
  tray_icon_->Focus();
}

void Tray::PopUpContextMenu(gin_helper::Arguments* args) {
  gin::Handle<Menu> menu;
  args->GetNext(&menu);
  gfx::Point pos;
  args->GetNext(&pos);
  tray_icon_->PopUpContextMenu(pos, menu.IsEmpty() ? nullptr : menu->model());
}

void Tray::CloseContextMenu() {
  tray_icon_->CloseContextMenu();
}

void Tray::SetContextMenu(gin_helper::ErrorThrower thrower,
                          v8::Local<v8::Value> arg) {
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
  return tray_icon_->GetBounds();
}

// static
void Tray::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "Tray"));
  gin_helper::Destroyable::MakeDestroyable(isolate, prototype);
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
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
      .SetMethod("getBounds", &Tray::GetBounds);
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::Tray;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  Tray::SetConstructor(isolate, base::BindRepeating(&Tray::New));

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set(
      "Tray",
      Tray::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_tray, Initialize)
