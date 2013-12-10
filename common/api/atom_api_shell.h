// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_SHELL_H_
#define ATOM_COMMON_API_ATOM_API_SHELL_H_

#include "base/basictypes.h"
#include "v8/include/v8.h"

namespace atom {

namespace api {

class Shell {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

 private:
  static v8::Handle<v8::Value> ShowItemInFolder(const v8::Arguments& args);
  static v8::Handle<v8::Value> OpenItem(const v8::Arguments& args);
  static v8::Handle<v8::Value> OpenExternal(const v8::Arguments& args);
  static v8::Handle<v8::Value> MoveItemToTrash(const v8::Arguments& args);
  static v8::Handle<v8::Value> Beep(const v8::Arguments& args);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Shell);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_SHELL_H_
