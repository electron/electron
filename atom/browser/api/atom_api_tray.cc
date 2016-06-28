// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_tray.h"

#include <string>

#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/browser.h"
#include "atom/browser/ui/tray_icon.h"
#include "atom/common/api/atom_api_native_image.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/node_includes.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/image/image.h"

namespace atom {

namespace api {

Tray::Tray(v8::Isolate* isolate, mate::Handle<NativeImage> image)
    : tray_icon_(TrayIcon::Create()) {
  SetImage(isolate, image);
  tray_icon_->AddObserver(this);
}

Tray::~Tray() {
}

// static
mate::WrappableBase* Tray::New(v8::Isolate* isolate,
                               mate::Handle<NativeImage> image) {
  if (!Browser::Get()->is_ready()) {
    isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
        isolate, "Cannot create Tray before app is ready")));
    return nullptr;
  }
  return new Tray(isolate, image);
}

void Tray::OnClicked(const gfx::Rect& bounds, int modifiers) {
  EmitWithFlags("click", modifiers, bounds);
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

void Tray::OnDragEntered() {
  Emit("drag-enter");
}

void Tray::OnDragExited() {
  Emit("drag-leave");
}

void Tray::OnDragEnded() {
  Emit("drag-end");
}

void Tray::SetImage(v8::Isolate* isolate, mate::Handle<NativeImage> image) {
#if defined(OS_WIN)
  tray_icon_->SetImage(image->GetHICON(GetSystemMetrics(SM_CXSMICON)));
#else
  tray_icon_->SetImage(image->image());
#endif
}

void Tray::SetPressedImage(v8::Isolate* isolate,
                           mate::Handle<NativeImage> image) {
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
  tray_icon_->SetTitle(title);
}

void Tray::SetHighlightMode(bool highlight) {
  tray_icon_->SetHighlightMode(highlight);
}

void Tray::DisplayBalloon(mate::Arguments* args,
                          const mate::Dictionary& options) {
  mate::Handle<NativeImage> icon;
  options.Get("icon", &icon);
  base::string16 title, content;
  if (!options.Get("title", &title) ||
      !options.Get("content", &content)) {
    args->ThrowError("'title' and 'content' must be defined");
    return;
  }

#if defined(OS_WIN)
  tray_icon_->DisplayBalloon(
      icon.IsEmpty() ? NULL : icon->GetHICON(GetSystemMetrics(SM_CXSMICON)),
      title, content);
#else
  tray_icon_->DisplayBalloon(
      icon.IsEmpty() ? gfx::Image() : icon->image(), title, content);
#endif
}

void Tray::PopUpContextMenu(mate::Arguments* args) {
  mate::Handle<Menu> menu;
  args->GetNext(&menu);
  gfx::Point pos;
  args->GetNext(&pos);
  tray_icon_->PopUpContextMenu(pos, menu.IsEmpty() ? nullptr : menu->model()->GetTrayModel());
}

void Tray::SetContextMenu(v8::Isolate* isolate, mate::Handle<Menu> menu) {
  menu_.Reset(isolate, menu.ToV8());
  tray_icon_->SetContextMenu(menu->model()->GetTrayModel());
}

gfx::Rect Tray::GetBounds() {
  return tray_icon_->GetBounds();
}

// static
void Tray::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .MakeDestroyable()
      .SetMethod("setImage", &Tray::SetImage)
      .SetMethod("setPressedImage", &Tray::SetPressedImage)
      .SetMethod("setToolTip", &Tray::SetToolTip)
      .SetMethod("setTitle", &Tray::SetTitle)
      .SetMethod("setHighlightMode", &Tray::SetHighlightMode)
      .SetMethod("displayBalloon", &Tray::DisplayBalloon)
      .SetMethod("popUpContextMenu", &Tray::PopUpContextMenu)
      .SetMethod("setContextMenu", &Tray::SetContextMenu)
      .SetMethod("getBounds", &Tray::GetBounds);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  using atom::api::Tray;
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Function> constructor = mate::CreateConstructor<Tray>(
      isolate, "Tray", base::Bind(&Tray::New));
  mate::Dictionary dict(isolate, exports);
  dict.Set("Tray", static_cast<v8::Local<v8::Value>>(constructor));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_tray, Initialize)
