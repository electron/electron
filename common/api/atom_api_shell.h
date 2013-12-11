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
  static void ShowItemInFolder(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void OpenItem(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void OpenExternal(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void MoveItemToTrash(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Beep(const v8::FunctionCallbackInfo<v8::Value>& args);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Shell);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_SHELL_H_
