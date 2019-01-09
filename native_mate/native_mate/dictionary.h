// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_DICTIONARY_H_
#define NATIVE_MATE_DICTIONARY_H_

#include "native_mate/converter.h"
#include "native_mate/object_template_builder.h"

namespace mate {

namespace internal {

// Returns true if |maybe| is both a value, and that value is true.
inline bool IsTrue(v8::Maybe<bool> maybe) {
  return maybe.IsJust() && maybe.FromJust();
}

}  // namespace internal

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
  Dictionary(v8::Isolate* isolate, v8::Local<v8::Object> object);
  Dictionary(const Dictionary& other);
  virtual ~Dictionary();

  static Dictionary CreateEmpty(v8::Isolate* isolate);

  template <typename T>
  bool Get(const base::StringPiece& key, T* out) const {
    // Check for existence before getting, otherwise this method will always
    // returns true when T == v8::Local<v8::Value>.
    v8::Local<v8::Context> context = isolate_->GetCurrentContext();
    v8::Local<v8::String> v8_key = StringToV8(isolate_, key);
    if (!internal::IsTrue(GetHandle()->Has(context, v8_key)))
      return false;

    v8::Local<v8::Value> val;
    if (!GetHandle()->Get(context, v8_key).ToLocal(&val))
      return false;
    return ConvertFromV8(isolate_, val, out);
  }

  template <typename T>
  bool GetHidden(const base::StringPiece& key, T* out) const {
    v8::Local<v8::Context> context = isolate_->GetCurrentContext();
    v8::Local<v8::Private> privateKey =
        v8::Private::ForApi(isolate_, StringToV8(isolate_, key));
    v8::Local<v8::Value> value;
    v8::Maybe<bool> result = GetHandle()->HasPrivate(context, privateKey);
    if (internal::IsTrue(result) &&
        GetHandle()->GetPrivate(context, privateKey).ToLocal(&value))
      return ConvertFromV8(isolate_, value, out);
    return false;
  }

  template <typename T>
  bool Set(const base::StringPiece& key, const T& val) {
    v8::Local<v8::Value> v8_value;
    if (!TryConvertToV8(isolate_, val, &v8_value))
      return false;
    v8::Maybe<bool> result = GetHandle()->Set(
        isolate_->GetCurrentContext(), StringToV8(isolate_, key), v8_value);
    return !result.IsNothing() && result.FromJust();
  }

  template <typename T>
  bool SetHidden(const base::StringPiece& key, T val) {
    v8::Local<v8::Value> v8_value;
    if (!TryConvertToV8(isolate_, val, &v8_value))
      return false;
    v8::Local<v8::Context> context = isolate_->GetCurrentContext();
    v8::Local<v8::Private> privateKey =
        v8::Private::ForApi(isolate_, StringToV8(isolate_, key));
    v8::Maybe<bool> result =
        GetHandle()->SetPrivate(context, privateKey, v8_value);
    return !result.IsNothing() && result.FromJust();
  }

  template <typename T>
  bool SetReadOnly(const base::StringPiece& key, T val) {
    v8::Local<v8::Value> v8_value;
    if (!TryConvertToV8(isolate_, val, &v8_value))
      return false;
    v8::Maybe<bool> result = GetHandle()->DefineOwnProperty(
        isolate_->GetCurrentContext(), StringToV8(isolate_, key), v8_value,
        v8::ReadOnly);
    return !result.IsNothing() && result.FromJust();
  }

  template <typename T>
  bool SetMethod(const base::StringPiece& key, const T& callback) {
    return GetHandle()->Set(
        StringToV8(isolate_, key),
        CallbackTraits<T>::CreateTemplate(isolate_, callback)
            ->GetFunction(isolate_->GetCurrentContext())
            .ToLocalChecked());
  }

  bool Delete(const base::StringPiece& key) {
    v8::Maybe<bool> result = GetHandle()->Delete(isolate_->GetCurrentContext(),
                                                 StringToV8(isolate_, key));
    return !result.IsNothing() && result.FromJust();
  }

  bool IsEmpty() const { return isolate() == NULL; }

  virtual v8::Local<v8::Object> GetHandle() const;

  v8::Isolate* isolate() const { return isolate_; }

 protected:
  v8::Isolate* isolate_;

 private:
  v8::Local<v8::Object> object_;
};

template <>
struct Converter<Dictionary> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, Dictionary val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     Dictionary* out);
};

}  // namespace mate

#endif  // NATIVE_MATE_DICTIONARY_H_
