// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_BINDINGS_
#define ATOM_BROWSER_API_ATOM_BINDINGS_

#include "base/basictypes.h"
#include "v8/include/v8.h"

namespace atom {

class AtomBindings {
 public:
  AtomBindings();
  virtual ~AtomBindings();

  // Add process.atom_binding function, which behaves like process.binding but
  // load native code from atom-shell instead.
  void BindTo(v8::Handle<v8::Object> process);

 private:
  static v8::Handle<v8::Value> Binding(const v8::Arguments& args);

  static v8::Persistent<v8::Object> binding_cache_;

  DISALLOW_COPY_AND_ASSIGN(AtomBindings);
};

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_BINDINGS_
