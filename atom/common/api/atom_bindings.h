// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_BINDINGS_H_
#define ATOM_COMMON_API_ATOM_BINDINGS_H_

#include <list>

#include "base/strings/string16.h"
#include "v8/include/v8.h"
#include "vendor/node/deps/uv/include/uv.h"

namespace node {
class Environment;
}

namespace atom {

class AtomBindings {
 public:
  AtomBindings();
  virtual ~AtomBindings();

  // Add process.atomBinding function, which behaves like process.binding but
  // load native code from atom-shell instead.
  void BindTo(v8::Isolate* isolate, v8::Local<v8::Object> process);

 private:
  void ActivateUVLoop(v8::Isolate* isolate);

  static void OnCallNextTick(uv_async_t* handle);

  uv_async_t call_next_tick_async_;
  std::list<node::Environment*> pending_next_ticks_;

  DISALLOW_COPY_AND_ASSIGN(AtomBindings);
};

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_BINDINGS_H_
