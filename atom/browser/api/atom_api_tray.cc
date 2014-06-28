// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_tray.h"

#include <string>

#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/ui/tray_icon.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

Tray::Tray(const gfx::ImageSkia& image)
    : tray_icon_(TrayIcon::Create()) {
  tray_icon_->SetImage(image);
  tray_icon_->AddObserver(this);
}

Tray::~Tray() {
}

// static
mate::Wrappable* Tray::New(const gfx::ImageSkia& image) {
  return new Tray(image);
}

void Tray::OnClicked() {
  Emit("clicked");
}

void Tray::SetImage(const gfx::ImageSkia& image) {
  tray_icon_->SetImage(image);
}

void Tray::SetPressedImage(const gfx::ImageSkia& image) {
  tray_icon_->SetPressedImage(image);
}

void Tray::SetToolTip(const std::string& tool_tip) {
  tray_icon_->SetToolTip(tool_tip);
}

void Tray::SetContextMenu(Menu* menu) {
  tray_icon_->SetContextMenu(menu->model());
}

// static
void Tray::BuildPrototype(v8::Isolate* isolate,
                          v8::Handle<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("setImage", &Tray::SetImage)
      .SetMethod("setPressedImage", &Tray::SetPressedImage)
      .SetMethod("setToolTip", &Tray::SetToolTip)
      .SetMethod("_setContextMenu", &Tray::SetContextMenu);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Handle<v8::Object> exports) {
  using atom::api::Tray;
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Handle<v8::Function> constructor = mate::CreateConstructor<Tray>(
      isolate, "Tray", base::Bind(&Tray::New));
  mate::Dictionary dict(isolate, exports);
  dict.Set("Tray", static_cast<v8::Handle<v8::Value>>(constructor));
}

}  // namespace

NODE_MODULE_X(atom_browser_tray, Initialize, NULL, NM_F_BUILTIN)
