// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "native_mate/converter.h"

namespace ui {
class Accelerator;
}

namespace mate {

template<>
struct Converter<ui::Accelerator> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     ui::Accelerator* out);
};

}  // namespace mate
