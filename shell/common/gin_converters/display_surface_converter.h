// Copyright (c) 2024 Slack, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_DISPLAY_SOURCE_CONVERTER_H_
#define ELECTRON_SHELL_BROWSER_API_DISPLAY_SOURCE_CONVERTER_H_

#include <memory>
#include <string>

#include "gin/converter.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom.h"

namespace electron {

// cf: 
// Chrome also allows browser here, but Electron does not.
enum PreferredDisplaySurface {
  NO_PREFERENCE,
  MONITOR,
  WINDOW,
};

}  // namespace electron

namespace gin {

template <>
struct Converter<blink::mojom::PreferredDisplaySurface> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   blink::mojom::PreferredDisplaySurface val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_BROWSER_DISPLAY_SOURCE_CONVERTER_H_