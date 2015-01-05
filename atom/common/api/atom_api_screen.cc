// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/node_includes.h"

namespace {

const char* kRequireAppReadyMessage =
    "APIs of \"screen\" module can only be used after the \"ready\" event of "
    "\"app\" module gets emitted.";

gfx::Point GetCursorScreenPoint(mate::Arguments* args) {
  if (!atom::Browser::Get()->is_ready()) {
    args->ThrowError(kRequireAppReadyMessage);
    return gfx::Point();
  }

  gfx::Screen* screen = gfx::Screen::GetNativeScreen();
  return screen->GetCursorScreenPoint();
}

gfx::Display GetPrimaryDisplay(mate::Arguments* args) {
  if (!atom::Browser::Get()->is_ready()) {
    args->ThrowError(kRequireAppReadyMessage);
    return gfx::Display();
  }

  gfx::Screen* screen = gfx::Screen::GetNativeScreen();
  return screen->GetPrimaryDisplay();
}

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCursorScreenPoint", &GetCursorScreenPoint);
  dict.SetMethod("getPrimaryDisplay", &GetPrimaryDisplay);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_screen, Initialize)
