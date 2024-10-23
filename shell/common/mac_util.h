// Copyright (c) 2024 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_MAC_UTIL_H_
#define ELECTRON_SHELL_MAC_UTIL_H_

#include "base/containers/span.h"

@class NSData;

namespace electron::util {

base::span<const uint8_t> as_byte_span(NSData* data);

}  // namespace electron::util

#endif  // ELECTRON_SHELL_MAC_UTIL_H_
