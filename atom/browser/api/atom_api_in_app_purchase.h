// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_
#define ATOM_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_

#include <string>

#include "atom/browser/in_app_purchase.h"
#include "atom/browser/in_app_purchase_observer.h"
#include "native_mate/dictionary.h"

namespace mate {

template <>
struct Converter<in_app_purchase::Payment> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const in_app_purchase::Payment& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     in_app_purchase::Payment* out);
};

template <>
struct Converter<in_app_purchase::Transaction> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const in_app_purchase::Transaction& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     in_app_purchase::Transaction* out);
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_
