// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/function_template.h"

#include "base/strings/strcat.h"

namespace gin_helper {

MaybeMicrotasksScope::MaybeMicrotasksScope(gin::Arguments* args) {
  if (args->isolate()->GetMicrotasksPolicy() !=
      v8::MicrotasksPolicy::kExplicit) {
    scope_.emplace(args->GetHolderCreationContext(),
                   v8::MicrotasksScope::kRunMicrotasks);
  }
}

MaybeMicrotasksScope::~MaybeMicrotasksScope() = default;

void ThrowConversionError(gin::Arguments* args,
                          const InvokerOptions& invoker_options,
                          size_t index) {
  if (index == 0 && invoker_options.holder_is_first_argument) {
    // Failed to get the appropriate `this` object. Either the native object
    // behind it has been destroyed - its wrapper then no longer converts - or
    // the method was invoked using Function.prototype.[call|apply] with an
    // invalid (or null) `this` argument. Telling the two apart here, after
    // the conversion has failed, keeps the check off every successful call.
    v8::Local<v8::Object> holder;
    if (args->GetHolder(&holder) && Destroyable::IsDestroyed(holder)) {
      args->ThrowTypeError("Object has been destroyed");
      return;
    }
    std::string error =
        invoker_options.holder_type
            ? base::StrCat({"Illegal invocation: Function must be "
                            "called on an object of type ",
                            invoker_options.holder_type})
            : "Illegal invocation";
    args->ThrowTypeError(error);
  } else {
    // Otherwise, this failed parsing on a different argument.
    // Arguments::ThrowError() will try to include appropriate information.
    // Ideally we would include the expected c++ type in the error message
    // here, too (which we can access via typeid(ArgType).name()), however we
    // compile with no-rtti, which disables typeid.
    args->ThrowError();
  }
}

}  // namespace gin_helper
