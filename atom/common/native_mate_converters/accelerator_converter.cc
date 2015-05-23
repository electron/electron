// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/accelerator_converter.h"

#include <string>

#include "atom/browser/ui/accelerator_util.h"

namespace mate {

// static
bool Converter<ui::Accelerator>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val, ui::Accelerator* out) {
  std::string keycode;
  if (!ConvertFromV8(isolate, val, &keycode))
    return false;
  return accelerator_util::StringToAccelerator(keycode, out);
}

}  // namespace mate
