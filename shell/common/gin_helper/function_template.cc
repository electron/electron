// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/function_template.h"

#include "base/strings/strcat.h"

namespace gin_helper {

CallbackHolderBase::CallbackHolderBase(uintptr_t type_identifier)
    : type_identifier_(type_identifier) {}

CallbackHolderBase::~CallbackHolderBase() = default;

const gin::WrapperInfo* CallbackHolderBase::wrapper_info() const {
  return &kWrapperInfo;
}

const char* CallbackHolderBase::GetHumanReadableName() const {
  return "Electron / CallbackHolder";
}

void ThrowConversionError(gin::Arguments* args,
                          const InvokerOptions& invoker_options,
                          size_t index) {
  if (index == 0 && invoker_options.holder_is_first_argument) {
    // Failed to get the appropriate `this` object. This can happen if a
    // method is invoked using Function.prototype.[call|apply] and passed an
    // invalid (or null) `this` argument.
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
