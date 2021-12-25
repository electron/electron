// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_

#include "gin/converter.h"
#include "shell/common/gin_helper/accessor.h"

namespace content {
class RenderFrameHost;
}

namespace gin {

template <>
struct Converter<content::RenderFrameHost*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::RenderFrameHost* val);
};

template <>
struct Converter<gin_helper::AccessorValue<content::RenderFrameHost*>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      gin_helper::AccessorValue<content::RenderFrameHost*> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gin_helper::AccessorValue<content::RenderFrameHost*>* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_
