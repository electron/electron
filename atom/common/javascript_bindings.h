// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_JAVASCRIPT_BINDINGS_H_
#define ATOM_COMMON_JAVASCRIPT_BINDINGS_H_

#include "extensions/renderer/object_backed_native_handler.h"
#include "extensions/renderer/script_context.h"
#include "v8/include/v8.h"

namespace atom {

class JavascriptBindings : public extensions::ObjectBackedNativeHandler {
 public:
  explicit JavascriptBindings(extensions::ScriptContext* context);
  virtual ~JavascriptBindings();

  void GetBinding(const v8::FunctionCallbackInfo<v8::Value>& args);
};

}  // namespace atom

#endif  // ATOM_COMMON_JAVASCRIPT_BINDINGS_H_
