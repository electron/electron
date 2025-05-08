// Copyright 2014 Cheng Zhao. All rights reserved.
// Use of this source code is governed by MIT license that can be found in the
// LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_PERSISTENT_DICTIONARY_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_PERSISTENT_DICTIONARY_H_

#include "base/memory/raw_ptr.h"
#include "gin/converter.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-object.h"

namespace gin_helper {

// Like Dictionary, but stores object in persistent handle so you can keep it
// safely on heap.
//
// TODO(zcbenz): The only user of this class is ElectronTouchBar, we should
// migrate away from this class.
class PersistentDictionary {
 public:
  PersistentDictionary();
  PersistentDictionary(v8::Isolate* isolate, v8::Local<v8::Object> object);
  PersistentDictionary(const PersistentDictionary& other);
  ~PersistentDictionary();

  PersistentDictionary& operator=(const PersistentDictionary& other);

  v8::Local<v8::Object> GetHandle() const;

  template <typename K, typename V>
  bool Get(const K& key, V* out) const {
    const auto handle = GetHandle();
    v8::Local<v8::Context> context = isolate_->GetCurrentContext();
    v8::Local<v8::Value> v8_key = gin::ConvertToV8(isolate_, key);
    v8::Local<v8::Value> value;
    return handle->Has(context, v8_key).FromMaybe(false) &&
           handle->Get(context, v8_key).ToLocal(&value) &&
           gin::ConvertFromV8(isolate_, value, out);
  }

  // Convenience function for using a default value if the
  // specified key isn't present in the dictionary.
  template <typename T>
  T ValueOrDefault(const std::string_view key, T default_value) const {
    if (auto value = T{}; Get(key, &value))
      return value;
    return default_value;
  }

 private:
  raw_ptr<v8::Isolate> isolate_ = nullptr;
  v8::Global<v8::Object> handle_;
};

}  // namespace gin_helper

namespace gin {

template <>
struct Converter<gin_helper::PersistentDictionary> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gin_helper::PersistentDictionary* out) {
    if (!val->IsObject())
      return false;
    *out = gin_helper::PersistentDictionary(isolate, val.As<v8::Object>());
    return true;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_PERSISTENT_DICTIONARY_H_
