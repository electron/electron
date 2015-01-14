// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/gfx_converter.h"
#include "base/bind.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/screen.h"

#include "atom/common/node_includes.h"

namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  auto screen = base::Unretained(gfx::Screen::GetNativeScreen());
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCursorScreenPoint",
                 base::Bind(&gfx::Screen::GetCursorScreenPoint, screen));
  dict.SetMethod("getPrimaryDisplay",
                 base::Bind(&gfx::Screen::GetPrimaryDisplay, screen));
  dict.SetMethod("getAllDisplays",
                 base::Bind(&gfx::Screen::GetAllDisplays, screen));
  dict.SetMethod("getDisplayNearestPoint",
                 base::Bind(&gfx::Screen::GetDisplayNearestPoint, screen));
  dict.SetMethod("getDisplayMatching",
                 base::Bind(&gfx::Screen::GetDisplayMatching, screen));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_screen, Initialize)
