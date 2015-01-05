// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/node_includes.h"

namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  gfx::Screen* screen = gfx::Screen::GetNativeScreen();
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCursorScreenPoint",
                 base::Bind(&gfx::Screen::GetCursorScreenPoint,
                            base::Unretained(screen)));
  dict.SetMethod("getPrimaryDisplay",
                 base::Bind(&gfx::Screen::GetPrimaryDisplay,
                            base::Unretained(screen)));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_screen, Initialize)
