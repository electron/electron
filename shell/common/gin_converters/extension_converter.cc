// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/extension_converter.h"

#include "extensions/common/extension.h"
#include "gin/dictionary.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/extensions/electron_extension_info.h"
#include "shell/browser/extensions/electron_extension_system.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"

namespace gin {

// static
v8::Local<v8::Value> Converter<extensions::ElectronExtensionInfo>::ToV8(
    v8::Isolate* isolate,
    const extensions::ElectronExtensionInfo& info) {
  auto extension_id = info.extension->id();
  auto dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("id", extension_id);
  dict.Set("name", info.extension->name());
  dict.Set("path", info.extension->path());
  dict.Set("url", info.extension->url());
  dict.Set("version", info.extension->VersionString());
  dict.Set("manifest", *info.extension->manifest()->value());

  auto* ext_system = info.browser_context->extension_system();
  dict.Set("enabled", ext_system->IsExtensionEnabled(extension_id));

  return gin::ConvertToV8(isolate, dict);
}

}  // namespace gin
