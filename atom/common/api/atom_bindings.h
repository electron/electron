// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_BINDINGS_
#define ATOM_COMMON_API_ATOM_BINDINGS_

#include "base/basictypes.h"
#include "v8/include/v8.h"

namespace atom {

class AtomBindings {
 public:
  AtomBindings();
  virtual ~AtomBindings();

  // Add process.atom_binding function, which behaves like process.binding but
  // load native code from atom-shell instead.
  virtual void BindTo(v8::Handle<v8::Object> process);

 private:
  static void Binding(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Crash(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ActivateUVLoop(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Log(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetCurrentStackTrace(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ScheduleCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

  DISALLOW_COPY_AND_ASSIGN(AtomBindings);
};

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_BINDINGS_
