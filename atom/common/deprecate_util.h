// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_DEPRECATE_UTIL_H_
#define ATOM_COMMON_DEPRECATE_UTIL_H_

#include <string>

#include "atom/common/node_includes.h"

namespace atom {

v8::Maybe<bool> EmitDeprecationWarning(node::Environment* env,
                                       std::string warning_msg,
                                       std::string warning_type,
                                       std::string warning_code);

}  // namespace atom

#endif  // ATOM_COMMON_DEPRECATE_UTIL_H_
