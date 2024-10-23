// Copyright (c) 2024 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/containers/span.h"

namespace electron::util {

base::span<const uint8_t> as_byte_span(NSData* data) {
  // SAFETY: There is no NSData API that passes the UNSAFE_BUFFER_USAGE
  // test, so let's isolate the unsafe API use into this function. Instead of
  // calling '[data bytes]' and '[data length]' directly, the rest of our
  // code should prefer to use spans returned by this function.
  return UNSAFE_BUFFERS(base::span{
      reinterpret_cast<const uint8_t*>([data bytes]), [data length]});
}

}  // namespace electron::util
