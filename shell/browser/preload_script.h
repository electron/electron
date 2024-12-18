// Copyright (c) 2024 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_
#define ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_

#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/files/file_path.h"
#include "base/uuid.h"
#include "gin/converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"

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
                                   const PreloadScript::ScriptType& in) {
    using Val = PreloadScript::ScriptType;
    static constexpr auto Lookup =
        base::MakeFixedFlatMap<Val, std::string_view>({
            {Val::kWebFrame, "frame"},
            {Val::kServiceWorker, "service-worker"},
        });
    return StringToV8(isolate, Lookup.at(in));
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     PreloadScript::ScriptType* out) {
    using Val = PreloadScript::ScriptType;
    static constexpr auto Lookup =
        base::MakeFixedFlatMap<std::string_view, Val>({
            {"frame", Val::kWebFrame},
            {"service-worker", Val::kServiceWorker},
        });
    return FromV8WithLookup(isolate, val, Lookup, out);
  }
};

template <>
struct Converter<PreloadScript> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const PreloadScript& script) {
    gin::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("filePath", script.file_path.AsUTF8Unsafe());
    dict.Set("id", script.id);
    dict.Set("type", script.script_type);
    return ConvertToV8(isolate, dict).As<v8::Object>();
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     PreloadScript* out) {
    gin_helper::Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;
    if (PreloadScript::ScriptType script_type;
        options.Get("type", &script_type)) {
      out->script_type = script_type;
    } else {
      return false;
    }
    if (base::FilePath file_path; options.Get("filePath", &file_path)) {
      out->file_path = file_path;
    } else {
      return false;
    }
    if (std::string id; options.Get("id", &id)) {
      out->id = id;
    } else {
      out->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
    }
    if (bool deprecated; options.Get("_deprecated", &deprecated)) {
      out->deprecated = deprecated;
    }
    return true;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_
