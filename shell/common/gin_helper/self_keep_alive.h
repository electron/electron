// Copyright (c) 2025 Microsoft, GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_SELF_KEEP_ALIVE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_SELF_KEEP_ALIVE_H_

#include "gin/weak_cell.h"

namespace gin_helper {

// Based on third_party/blink/renderer/platform/heap/self_keep_alive.h
template <typename Self>
class SelfKeepAlive final {
  GIN_DISALLOW_NEW();

 public:
  explicit SelfKeepAlive(Self* self) : keep_alive_(self) {}

  SelfKeepAlive& operator=(Self* self) {
    DCHECK(!keep_alive_ || keep_alive_.Get() == self);
    keep_alive_ = self;
    return *this;
  }

  void Clear() { keep_alive_.Clear(); }

  explicit operator bool() const { return keep_alive_; }

 private:
  cppgc::Persistent<Self> keep_alive_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_SELF_KEEP_ALIVE_H_
