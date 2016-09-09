// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_RENDERER_EXTENSIONS_CONTENT_SETTINGS_BINDINGS_H_
#define BRAVE_RENDERER_EXTENSIONS_CONTENT_SETTINGS_BINDINGS_H_

#include "extensions/renderer/object_backed_native_handler.h"
#include "extensions/renderer/script_context.h"
#include "v8/include/v8.h"

namespace brave {

class ContentSettingsBindings : public extensions::ObjectBackedNativeHandler {
 public:
  explicit ContentSettingsBindings(extensions::ScriptContext* context);
  virtual ~ContentSettingsBindings();

  void GetCurrentSetting(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  void GetContentTypes(
      const v8::FunctionCallbackInfo<v8::Value>& args);
 private:
  DISALLOW_COPY_AND_ASSIGN(ContentSettingsBindings);
};

}  // namespace brave

#endif  // BRAVE_RENDERER_EXTENSIONS_CONTENT_SETTINGS_BINDINGS_H_
