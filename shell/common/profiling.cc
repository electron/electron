// Copyright (c) 2019 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <vector>

#include "electron/shell/common/gin_helper/dictionary.h"
#include "electron/shell/common/profiling.h"

namespace gin {

template <>
struct Converter<base::Time> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Time& val) {
    v8::MaybeLocal<v8::Value> date =
        v8::Date::New(isolate->GetCurrentContext(), val.ToJsTime());
    if (date.IsEmpty())
      return v8::Null(isolate);
    return date.ToLocalChecked();
  }
};

}  // namespace gin

namespace electron {

namespace profiling {

std::vector<std::pair<std::string, base::Time>> marks;

void Mark(base::StringPiece key) {
  marks.emplace_back(key.as_string(), base::Time::Now());
}

}  // namespace profiling

v8::Local<v8::Value> GetProfilingMarks(v8::Isolate* isolate) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  for (const auto& it : profiling::marks) {
    dict.Set(it.first, it.second);
  }
  return gin::ConvertToV8(isolate, dict);
}

}  // namespace electron
