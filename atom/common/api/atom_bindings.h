// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_BINDINGS_H_
#define ATOM_COMMON_API_ATOM_BINDINGS_H_

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "v8/include/v8.h"

namespace atom {

class AtomBindings {
 public:
  AtomBindings();
  virtual ~AtomBindings();

  // Add process.atom_binding function, which behaves like process.atomBinding but
  // load native code from atom-shell instead.
  virtual void BindTo(v8::Isolate* isolate, v8::Handle<v8::Object> process);

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomBindings);
};

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_BINDINGS_H_
