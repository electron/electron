// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_CONSTRUCTOR_H_
#define ATOM_COMMON_API_CONSTRUCTOR_H_

#include "native_mate/constructor.h"

namespace mate {

// Create a FunctionTemplate that can be "new"ed in JavaScript.
// It is user's responsibility to ensure this function is called for one type
// only ONCE in the program's whole lifetime, otherwise we would have memory
// leak.
template <typename T, typename Sig>
v8::Local<v8::Function> CreateConstructor(v8::Isolate* isolate,
                                          const base::Callback<Sig>& func) {
#ifndef NDEBUG
  static bool called = false;
  CHECK(!called) << "CreateConstructor can only be called for one type once";
  called = true;
#endif
  v8::Local<v8::FunctionTemplate> templ = CreateFunctionTemplate(
      isolate, base::Bind(&mate::internal::InvokeNew<Sig>, func));
  templ->InstanceTemplate()->SetInternalFieldCount(1);
  T::BuildPrototype(isolate, templ);
  return templ->GetFunction();
}

}  // namespace mate

#endif  // ATOM_COMMON_API_CONSTRUCTOR_H_
