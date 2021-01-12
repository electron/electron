// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_

#include "gin/converter.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace gin {

template <>
struct Converter<content::RenderFrameHost*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::RenderFrameHost* val);
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_
