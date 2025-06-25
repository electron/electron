// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_
#define ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_

#include <string>

#include "base/files/file_path.h"
#include "gin/converter.h"
#include "v8/include/v8-forward.h"

namespace electron {

struct PreloadScript {
  enum class ScriptType { kWebFrame, kServiceWorker };

  std::string id;
  ScriptType script_type;
  base::FilePath file_path;

  // If set, use the deprecated validation behavior of Session.setPreloads
  bool deprecated = false;
};

}  // namespace electron

namespace gin {

using electron::PreloadScript;

template <>
struct Converter<PreloadScript::ScriptType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const PreloadScript::ScriptType& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     PreloadScript::ScriptType* out);
};

template <>
struct Converter<PreloadScript> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const PreloadScript& script);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     PreloadScript* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_
