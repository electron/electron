// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_tray.h"

#include <string>

#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/browser.h"
#include "atom/browser/ui/tray_icon.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/image/image.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

Tray::Tray(const gfx::Image& image)
    : tray_icon_(TrayIcon::Create()) {
  tray_icon_->SetImage(image);
  tray_icon_->AddObserver(this);
}

Tray::~Tray() {
}

// static
mate::Wrappable* Tray::New(v8::Isolate* isolate, const gfx::Image& image) {
  if (!Browser::Get()->is_ready()) {
    node::ThrowError(isolate, "Cannot create Tray before app is ready");
    return nullptr;
  }
  return new Tray(image);
}

void Tray::OnClicked(const gfx::Rect& bounds) {
  Emit("clicked", bounds);
}

void Tray::OnDoubleClicked() {
  Emit("double-clicked");
}

void Tray::OnBalloonShow() {
  Emit("balloon-show");
}

void Tray::OnBalloonClicked() {
  Emit("balloon-clicked");
}

void Tray::OnBalloonClosed() {
  Emit("balloon-closed");
}

void Tray::Destroy() {
  tray_icon_.reset();
}

void Tray::SetImage(mate::Arguments* args, const gfx::Image& image) {
  if (!CheckTrayLife(args))
    return;
  tray_icon_->SetImage(image);
}

void Tray::SetPressedImage(mate::Arguments* args, const gfx::Image& image) {
  if (!CheckTrayLife(args))
    return;
  tray_icon_->SetPressedImage(image);
}

void Tray::SetToolTip(mate::Arguments* args, const std::string& tool_tip) {
  if (!CheckTrayLife(args))
    return;
  tray_icon_->SetToolTip(tool_tip);
}

void Tray::SetTitle(mate::Arguments* args, const std::string& title) {
  if (!CheckTrayLife(args))
    return;
  tray_icon_->SetTitle(title);
}

void Tray::SetHighlightMode(mate::Arguments* args, bool highlight) {
  if (!CheckTrayLife(args))
    return;
  tray_icon_->SetHighlightMode(highlight);
}

void Tray::DisplayBalloon(mate::Arguments* args,
                          const mate::Dictionary& options) {
  if (!CheckTrayLife(args))
    return;

  gfx::Image icon;
  options.Get("icon", &icon);
  base::string16 title, content;
  if (!options.Get("title", &title) ||
      !options.Get("content", &content)) {
    args->ThrowError("'title' and 'content' must be defined");
    return;
  }

  tray_icon_->DisplayBalloon(icon, title, content);
}

void Tray::SetContextMenu(mate::Arguments* args, Menu* menu) {
  if (!CheckTrayLife(args))
    return;
  tray_icon_->SetContextMenu(menu->model());
}

bool Tray::CheckTrayLife(mate::Arguments* args) {
  if (!tray_icon_) {
    args->ThrowError("Tray is already destroyed");
    return false;
  } else {
    return true;
  }
}

// static
void Tray::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("destroy", &Tray::Destroy)
      .SetMethod("setImage", &Tray::SetImage)
      .SetMethod("setPressedImage", &Tray::SetPressedImage)
      .SetMethod("setToolTip", &Tray::SetToolTip)
      .SetMethod("setTitle", &Tray::SetTitle)
      .SetMethod("setHighlightMode", &Tray::SetHighlightMode)
      .SetMethod("displayBalloon", &Tray::DisplayBalloon)
      .SetMethod("_setContextMenu", &Tray::SetContextMenu);
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
