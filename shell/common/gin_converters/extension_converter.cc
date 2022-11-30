// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/extension_converter.h"

#include "extensions/common/extension.h"
#include "gin/dictionary.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"

namespace gin {

// static
v8::Local<v8::Value> Converter<const extensions::Extension*>::ToV8(
    v8::Isolate* isolate,
    const extensions::Extension* extension) {
  auto dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("id", extension->id());
  dict.Set("name", extension->name());
  dict.Set("path", extension->path());
  dict.Set("url", extension->url());
  dict.Set("version", extension->VersionString());
  dict.Set("manifest",
           *static_cast<const base::Value*>(extension->manifest()->value()));

  return gin::ConvertToV8(isolate, dict);
}

}  // namespace gin
