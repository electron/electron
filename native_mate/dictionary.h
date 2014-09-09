// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_DICTIONARY_H_
#define NATIVE_MATE_DICTIONARY_H_

#include "native_mate/converter.h"
#include "native_mate/object_template_builder.h"

namespace mate {

// Dictionary is useful when writing bindings for a function that either
// receives an arbitrary JavaScript object as an argument or returns an
// arbitrary JavaScript object as a result. For example, Dictionary is useful
// when you might use the |dictionary| type in WebIDL:
//
//   http://heycam.github.io/webidl/#idl-dictionaries
//
// WARNING: You cannot retain a Dictionary object in the heap. The underlying
//          storage for Dictionary is tied to the closest enclosing
//          v8::HandleScope. Generally speaking, you should store a Dictionary
//          on the stack.
//
class Dictionary {
 public:
  Dictionary();
  Dictionary(v8::Isolate* isolate, v8::Handle<v8::Object> object);
  ~Dictionary();

  template<typename T>
  bool Get(const base::StringPiece& key, T* out) const {
    v8::Handle<v8::Value> val = GetHandle()->Get(StringToV8(isolate_, key));
    return ConvertFromV8(isolate_, val, out);
  }

  template<typename T>
  bool Set(const base::StringPiece& key, T val) {
    return GetHandle()->Set(StringToV8(isolate_, key),
                            ConvertToV8(isolate_, val));
  }

  template<typename T>
  bool SetMethod(const base::StringPiece& key, const T& callback) {
    return GetHandle()->Set(
        StringToV8(isolate_, key),
        CallbackTraits<T>::CreateTemplate(isolate_, callback)->GetFunction());
  }

  bool IsEmpty() const { return isolate() == NULL; }

  virtual v8::Handle<v8::Object> GetHandle() const;

  v8::Isolate* isolate() const { return isolate_; }

 protected:
  v8::Isolate* isolate_;

 private:
  v8::Handle<v8::Object> object_;
};

template<>
struct Converter<Dictionary> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    Dictionary val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     Dictionary* out);
};

}  // namespace mate

#endif  // NATIVE_MATE_DICTIONARY_H_
