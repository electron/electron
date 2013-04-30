// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_CLIPBOARD_H_
#define ATOM_COMMON_API_ATOM_API_CLIPBOARD_H_

#include "base/basictypes.h"
#include "v8/include/v8.h"

namespace atom {

namespace api {

class Clipboard {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

 private:
  static v8::Handle<v8::Value> Has(const v8::Arguments &args);
  static v8::Handle<v8::Value> Read(const v8::Arguments &args);
  static v8::Handle<v8::Value> ReadText(const v8::Arguments &args);
  static v8::Handle<v8::Value> WriteText(const v8::Arguments &args);
  static v8::Handle<v8::Value> Clear(const v8::Arguments &args);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Clipboard);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_CLIPBOARD_H_
