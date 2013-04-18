// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_bindings.h"

#include "base/logging.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

using node::node_isolate;

namespace atom {

// Defined in atom_api_extensions.cc.
namespace api {
node::node_module_struct* get_builtin_module(const char *name);
}

v8::Persistent<v8::Object> AtomBindings::binding_cache_;

AtomBindings::AtomBindings() {
}

AtomBindings::~AtomBindings() {
}

void AtomBindings::BindTo(v8::Handle<v8::Object> process) {
  v8::HandleScope scope;

  node::SetMethod(process, "atom_binding", Binding);
}

void AtomBindings::AfterLoad() {
  v8::HandleScope scope;

  v8::Handle<v8::Object> global = node::g_context->Global();
  v8::Handle<v8::Object> atom =
    global->Get(v8::String::New("__atom"))->ToObject();
  DCHECK(!atom.IsEmpty());

  browser_main_parts_ = v8::Persistent<v8::Object>::New(
      node_isolate,
      atom->Get(v8::String::New("browserMainParts"))->ToObject());
  DCHECK(!browser_main_parts_.IsEmpty());
}

// static
v8::Handle<v8::Value> AtomBindings::Binding(const v8::Arguments& args) {
  v8::HandleScope scope;

  v8::Local<v8::String> module = args[0]->ToString();
  v8::String::Utf8Value module_v(module);
  node::node_module_struct* modp;

  if (binding_cache_.IsEmpty()) {
    binding_cache_ = v8::Persistent<v8::Object>::New(
        node_isolate, v8::Object::New());
  }

  v8::Local<v8::Object> exports;

  if (binding_cache_->Has(module)) {
    exports = binding_cache_->Get(module)->ToObject();
    return scope.Close(exports);
  }

  if ((modp = api::get_builtin_module(*module_v)) != NULL) {
    exports = v8::Object::New();
    // Internal bindings don't have a "module" object,
    // only exports.
    modp->register_func(exports, v8::Undefined());
    binding_cache_->Set(module, exports);
    return scope.Close(exports);
  }

  return v8::ThrowException(v8::Exception::Error(
      v8::String::New("No such module")));
}

}  // namespace atom
