// Copyright (c) 2021 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_ACCESSOR_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_ACCESSOR_H_

#include "base/memory/raw_ptr_exclusion.h"

namespace gin_helper {

// Wrapper for a generic value to be used as an accessor in a
// gin_helper::Dictionary.
template <typename T>
struct AccessorValue {
  T Value;
};
template <typename T>
struct AccessorValue<const T&> {
  T Value;
};
template <typename T>
struct AccessorValue<const T*> {
  RAW_PTR_EXCLUSION T* Value;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_ACCESSOR_H_
