// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_JAVASCRIPT_BINDINGS_H_
#define ATOM_COMMON_JAVASCRIPT_BINDINGS_H_

#include "base/files/file_path.h"
#include "v8/include/v8.h"

namespace atom {

class JavascriptBindings {
 public:
  JavascriptBindings();
  virtual ~JavascriptBindings();

  static void PreSandboxStartup();

  void BindTo(v8::Isolate* isolate, v8::Local<v8::Object> process);

 private:
  DISALLOW_COPY_AND_ASSIGN(JavascriptBindings);
};

}  // namespace atom

#endif  // ATOM_COMMON_JAVASCRIPT_BINDINGS_H_
