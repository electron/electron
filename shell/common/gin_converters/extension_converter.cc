// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/extension_converter.h"

#include "extensions/common/extension.h"
#include "gin/dictionary.h"

namespace gin {

// static
v8::Local<v8::Value> Converter<const extensions::Extension*>::ToV8(
    v8::Isolate* isolate,
    const extensions::Extension* extension) {
  auto dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("id", extension->id());
  dict.Set("name", extension->name());
  dict.Set("version", extension->VersionString());
  return gin::ConvertToV8(isolate, dict);
}

}  // namespace gin
