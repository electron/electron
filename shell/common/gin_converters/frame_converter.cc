// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/frame_converter.h"

#include <string>
#include <vector>

#include "content/public/browser/render_frame_host.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"

namespace gin {

// static
v8::Local<v8::Value> Converter<content::RenderFrameHost*>::ToV8(
    v8::Isolate* isolate,
    content::RenderFrameHost* val) {
  if (!val)
    return v8::Null(isolate);
  return electron::api::WebFrameMain::From(isolate, val).ToV8();
}

}  // namespace gin
